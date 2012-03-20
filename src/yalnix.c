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

  dprintf("in set kernel brk", 0);
  printf("address value: %p\n", addr);

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
        dprintf("finished allocating memory", 0);
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

  /* TMP */
  args_copy = cmd_args; //TODO: hack!

  /* Get memory size*/
  num_frames = pmem_size / PAGESIZE;
  assert(num_frames > 0); // silly...

  /* Set the heap limit */
  KERNEL_HEAP_LIMIT = orig_brk; // top of the heap
  page_brk = VMEM_1_LIMIT - PAGESIZE; // top of region 1 table

  /* Create physical frames */
  dprintf("initializing frames...", 0);
  initialize_frames(num_frames);

  // create process queues
  p_ready = create_queue();
  p_waiting = create_queue();
  p_delay = create_queue();


  /* Initialize interrupt vector table */
  for (i=0; i < TRAP_VECTOR_SIZE; i++) {
    interrupt_vector_table[i] = NULL;
  }
  interrupt_vector_table[TRAP_KERNEL] = &interrupt_kernel;
  interrupt_vector_table[TRAP_CLOCK] = &interrupt_clock;
  interrupt_vector_table[TRAP_ILLEGAL] = &interrupt_illegal;
  interrupt_vector_table[TRAP_MEMORY] = &interrupt_memory;
  interrupt_vector_table[TRAP_MATH] = &interrupt_math;
  interrupt_vector_table[TRAP_TTY_RECEIVE] = &interrupt_kernel;
  interrupt_vector_table[TRAP_TTY_TRANSMIT] = &interrupt_kernel;
  interrupt_vector_table[TRAP_DISK] = &interrupt_kernel;

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
  page_table0_p = create_page_table();
  page_table0_p = init_page_table0(page_table0_p);

  /* Debug region values*/
  printf(DIVIDER);
  printf("pmem: %u\n", pmem_size);
  printf("free frames: %i\n", len_free_frames());
  printf(DIVIDER);
  debug_page_table(page_table0_p, 0);
  debug_page_table(page_table1_p, 0);
  printf(DIVIDER);
  printf("heap limit: %p\n", (void *)KERNEL_HEAP_LIMIT);
  printf("text limit: %p\n", &_etext);
  printf("vmem_1_base: %p\n", (void *)VMEM_1_BASE);
  printf(DIVIDER);
  printf("kernel stack limit: %p\n", (void *)KERNEL_STACK_LIMIT);
  printf("kernel stack base: %p\n", (void *)KERNEL_STACK_BASE);
  printf("vmem_0_base: %p\n", (void *)VMEM_0_BASE);
  printf(DIVIDER);
  printf("page frame addr: %p\n", frames_p);
  printf("vector table: %p\n", (void *)interrupt_vector_table);
  printf("page table0: %p\n", page_table0_p);
  printf("page table1: %p\n", page_table1_p);

  WriteRegister( REG_PTR0, (RCS421RegVal) page_table0_p);
  WriteRegister( REG_PTR1, (RCS421RegVal) page_table1_p);

  SavedContext *ctx_idle = (SavedContext *)malloc(sizeof(SavedContext));
  SavedContext *ctx_init = (SavedContext *)malloc(sizeof(SavedContext));

  /* Enable virtual memory */
  dprintf("enabling virtual memory...", 1);
  WriteRegister( REG_VM_ENABLE, (RCS421RegVal) 1);
  VM_ENABLED = true;

  // Create idle process
  /**************************************/
  printf(DIVIDER);
  printf("IDLE process\n");
  printf(DIVIDER);
  // Create an idle process
  dprintf("create idle process...", 1);
  /*struct pte *idle_page_table;*/
  pcb_idle = Create_pcb(NULL);
  pcb_idle->name = "idle process";

  // Set page table to initialized table
  pcb_idle->page_table_p = create_page_table();

  // Saved context
  dprintf("about to create context...", 0);
  pcb_idle->context = ctx_idle;
  /*debug_frames(0);*/

  // Initialzed context but don't actually context switch
  if ( 0 > ContextSwitch(switchfunc_idle, pcb_idle->context, (void *)pcb_idle, NULL)) unix_error("bad context switch!");
  dprintf("finished initializing idle...", 0);

  dprintf("about to load idle...", 0);
  /*if(LoadProgram("idle", cmd_args, frame, &pcb_idle) != 0) unix_error("error loading program!");*/
  debug_pcb(pcb_idle);

  printf(DIVIDER);
  printf("INIT process\n");
  printf(DIVIDER);
  // Load init
  /******************************************/
  // Create init program
  // Create pcb for init program
  struct pcb *pcb_init;

  // create pcb for init
  dprintf("creating init process...", 1);
  pcb_init = Create_pcb(NULL);
  pcb_init->name = "init process";

  // Initiate page table
  dprintf("creating page tables for init...", 0);
  pcb_init->page_table_p = create_page_table();

  // Saved context
  dprintf("about to create context...", 0);
  pcb_init->context = ctx_init;

  // Special context switch into init that inherits current process context
  dprintf("about to context switch...", 0);
  if (0 > ContextSwitch(switchfunc_init, pcb_init->context, (void *)pcb_init, NULL)) unix_error("error context switching!");
  debug_pcb(pcb_init);

  dprintf("about to load init...", 0);
  if(LoadProgram("init", cmd_args, frame, &pcb_init) != 0) unix_error("error loading program!");

  // Set init as current process
  pcb_current = pcb_init;
  pcb_current->pc_next = frame->pc;
  pcb_current->sp_next = frame->sp;
  pcb_current->psr_next = frame->psr;
  pcb_current->frame = frame;

  dprintf("finished loading init...", 0);
  dprintf("done starting kernel!", 0);
}

