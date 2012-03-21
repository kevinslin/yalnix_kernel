#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

#include "simpleutils.h"
#include "yalnix_interrupts.h"
#include "yalnix_mem.h"

/* Function headers */
SavedContext* initswitchfunction(SavedContext *ctxp, void *p1, void *p2);
void create_idle_pcb();
void start_idle();

/* Extern */
extern int LoadProgram(char *name, char **args, ExceptionStackFrame *frame, struct pcb **pcb_p);

/* Globals */
void *interrupt_vector_table[TRAP_VECTOR_SIZE];
char **args_copy;
static int runOnce = 0;
struct pte page_table1[PAGE_TABLE_LEN];
struct pte *page_table1_p = page_table1;

/* Kernel Functions */
/*
 * Manages memory of the kernel
 */
/*
    get the current page which should be the kernerlbrk
    get page of the addr we need to go to
    if the current page is less than the neededpage we need to free some memory TODO
    otherwise need to increase memory
    check to see if there is enough memory
    need to get the vpn address somehow in order to accurately update the pte
    make sure to TLB_FLUSH_1
*/
int SetKernelBrk(void *addr) {
  long kernel_heap_limit_index;
  long addr_index;
  int pages_needed;
  int i;
  int frame;

  // if vm enabled, then look for free frames to hold space
  if (VM_ENABLED) {
    //kernel_heap_limit_index = get_page_index(KERNEL_HEAP_LIMIT);
    kernel_heap_limit_index = UP_TO_PAGE(KERNEL_HEAP_LIMIT)/PAGESIZE;
    addr_index = UP_TO_PAGE(addr)/PAGESIZE;
    pages_needed = addr_index - kernel_heap_limit_index;
    // the kernel heap is not big enough to hold requested memory
    if (pages_needed > 0) {
      // check if we have enough physical emmory
      if (len_free_frames() >= pages_needed) {
        for (i = 0; i < pages_needed; i++) {
          frame = get_free_frame();
          (page_table1_p + kernel_heap_limit_index - TABLE1_OFFSET + i)->valid = PTE_VALID;
          (page_table1_p + kernel_heap_limit_index - TABLE1_OFFSET + i)->pfn = frame;
          (page_table1_p + kernel_heap_limit_index - TABLE1_OFFSET + i)->uprot = PROT_NONE;
          (page_table1_p + kernel_heap_limit_index - TABLE1_OFFSET + i)->kprot = (PROT_READ | PROT_WRITE);
          WriteRegister( REG_TLB_FLUSH, TLB_FLUSH_1);
        }
        // update kernel heap limit
        KERNEL_HEAP_LIMIT = addr;
        return 0;
      } else {
        // don't have enough frames
        return ERROR;
      }
    } else {
      // call to free
      pages_needed = kernel_heap_limit_index - addr_index; // heap higher than addr
      for (i = 0; i < pages_needed; i++) {
        // free'd pages are invalid
        (page_table1_p + kernel_heap_limit_index - TABLE1_OFFSET - i)->valid = PTE_INVALID;
        set_frame((page_table1_p + kernel_heap_limit_index - TABLE1_OFFSET - i)->pfn, FRAME_FREE);
        (page_table1_p + kernel_heap_limit_index - TABLE1_OFFSET - i)->pfn = 0;
        (page_table1_p + kernel_heap_limit_index - TABLE1_OFFSET - i)->uprot = PROT_NONE;
        (page_table1_p + kernel_heap_limit_index - TABLE1_OFFSET - i)->kprot = PROT_NONE;
        WriteRegister( REG_TLB_FLUSH, TLB_FLUSH_1);
      }
      KERNEL_HEAP_LIMIT = addr;
      return 0;
    } // end of pages_needed
  } else {
    // vm is disabled, just move brk pointer up
    KERNEL_HEAP_LIMIT = addr;
  }
  return 0;
}

/*
 * Called at kernel startup
 */
void KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args) {
	
	/* Init vars */
  int i;
  int num_frames;
  int kernel_heap_limit_index;
  int kernel_text_limit_index;

  args_copy = cmd_args; 

  /* Get memory size*/
  num_frames = pmem_size / PAGESIZE;
  
  /* Globals init */
  KERNEL_HEAP_LIMIT = orig_brk; // top of the heap
  page_brk = VMEM_1_LIMIT - PAGESIZE; // top of region 1 table
  IDLE_CREATED = false;

  /* Create physical frames */
  initialize_frames(num_frames);

  // create process queues
  p_ready = create_queue();
  p_waiting = create_queue();
  p_delay = create_queue();
  page_table_free = create_queue();
  int j;
  for(j = 0; j<4; j++){
	  tty_read[j] = create_queue();
	  tty_write[j] = create_queue();
	  tty_read_wait[j] = create_queue();
	  tty_write_wait[j] = create_queue();
	  tty_busy[j] = 0;
  }



  /* Initialize interrupt vector table */
  for (i=0; i < TRAP_VECTOR_SIZE; i++) {
    interrupt_vector_table[i] = NULL;
  }
  interrupt_vector_table[TRAP_KERNEL] = &interrupt_kernel;
  interrupt_vector_table[TRAP_CLOCK] = &interrupt_clock;
  interrupt_vector_table[TRAP_ILLEGAL] = &interrupt_illegal;
  interrupt_vector_table[TRAP_MEMORY] = &interrupt_memory;
  interrupt_vector_table[TRAP_MATH] = &interrupt_math;
  interrupt_vector_table[TRAP_TTY_RECEIVE] = &interrupt_tty_receive;
  interrupt_vector_table[TRAP_TTY_TRANSMIT] = &interrupt_tty_transmit;
  interrupt_vector_table[TRAP_DISK] = &interrupt_disk;

  /* Point REG_VECTOR_BASE at interupt vector table */
  WriteRegister( REG_VECTOR_BASE, (RCS421RegVal) &interrupt_vector_table);

  /* Initialize all page table entries to invalid */
  for (i=0; i< PAGE_TABLE_LEN; i++) {
    // Region 1
    (page_table1_p + i)->pfn = PFN_INVALID;
    (page_table1_p + i)->valid = PTE_INVALID;
    (page_table1_p + i)->uprot = PROT_NONE;
    (page_table1_p + i)->kprot = PROT_NONE;
  }

  /* Get kernel memory offsets */
  /*
   * ====================
   * Region 1
   * ====================
   * VMEM_1_LIMIT
   * ...
   * KERNEL_HEAP_INDEX (593)
   * ...
   * KERNEL_TEXT_LIMIT (589)
   * ...
   * VMEM_1_BASE
   * ==================
   * Region 0
   * ====================
   * KERNEL_STACK_LIMIT/VMEM_0_LIMIT (512)
   * ...
   * KERNEL_STACK_BASE (508)
   * ...
   * User Stack LIMIT
   * ...
   * MEM_INVALID
   * VMEM_0_BASE (0)
   */

  kernel_heap_limit_index = get_page_index(KERNEL_HEAP_LIMIT);
  kernel_text_limit_index = get_page_index(&_etext);

  /* Map physical pages to corresponding virtual addresses */
  // TABLE1_OFFSET neccessary to account for VMEM_1_BASE higher starting address
  for(i=kernel_text_limit_index; i <= kernel_heap_limit_index + 1; i++) {
    (page_table1_p + i - TABLE1_OFFSET)->valid = PTE_VALID;
    (page_table1_p + i - TABLE1_OFFSET)->pfn = i;
    (page_table1_p + i - TABLE1_OFFSET)->kprot = (PROT_READ | PROT_WRITE);
    set_frame(i, FRAME_NOT_FREE);
  }
  for(i=get_page_index(VMEM_1_BASE); i < kernel_text_limit_index; i++) {
    (page_table1_p + i - TABLE1_OFFSET)->valid = PTE_VALID;
    (page_table1_p + i - TABLE1_OFFSET)->pfn = i;
    (page_table1_p + i - TABLE1_OFFSET)->kprot = (PROT_READ | PROT_EXEC);
    set_frame(i, FRAME_NOT_FREE);
  }
  // Initialize kernel stack of pt 0 to valid
  page_table0_p = create_page_table_helper(NULL);
  page_table0_p = init_page_table0(page_table0_p);

  WriteRegister( REG_PTR0, (RCS421RegVal) page_table0_p);
  WriteRegister( REG_PTR1, (RCS421RegVal) page_table1_p);

  
  /* Enable virtual memory */
  WriteRegister( REG_VM_ENABLE, (RCS421RegVal) 1);
  VM_ENABLED = true;
  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

  // Create idle process
  /**************************************/
  /*// Create an idle process*/
  struct pcb *pcb_idle;
  pcb_idle = Create_pcb(NULL);
  

  /*// Set page table to initialized table*/
  pcb_idle->page_table_p = create_page_table();
  //pcb_idle->context = ctx_idle;
  // Saved context
  pcb_current = pcb_idle;
  SavedContext *ctx = &(pcb_idle->context);
  if ( 0 > ContextSwitch(switchfunc_init, ctx, (void *)pcb_idle, NULL)) unix_error("bad context switch!");

  if(LoadProgram("idle", cmd_args, frame, &pcb_idle) != 0) unix_error("error loading program!");
  pcb_idle->pc_next = frame->pc;
  pcb_idle->sp_next = frame->sp;
  pcb_idle->psr_next = frame->psr;
  pcb_idle->frame = frame;
  // Load init
  /******************************************/
  // Create init program
   //Create pcb for init program
  struct pcb *pcb_init;

  // create pcb for init
  pcb_init = Create_pcb(NULL);
  pcb_init->name = "write process";

  // Initiate page table
  pcb_init->page_table_p = create_page_table();

  // Saved context
  //pcb_init->context = ctx_init;
  // Special context switch into init that inherits current process 
  pcb_current = pcb_init;
  SavedContext *initctx = &(pcb_init->context);
  if (0 > ContextSwitch(switchfunc_init, initctx, (void *)pcb_init, NULL)) unix_error("error context switching!");
  if (runOnce == 0) 
        {
                runOnce = 1;
        }
        else {
                return;
        }
	page_table0_p = pcb_init->page_table_p;
	WriteRegister(REG_PTR0, (RCS421RegVal)page_table0_p);
	
	if(VM_ENABLED){
		WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
	}
  if(LoadProgram(cmd_args[0], cmd_args, frame, &pcb_init) != 0) unix_error("error loading program!");

  test = pcb_init;
  pcb_current = pcb_init;
  idle_pcb = pcb_idle;

}
