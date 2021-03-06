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
bool VM_ENABLED;
char **args_copy;

struct pte page_table1[PAGE_TABLE_LEN];
struct pte *page_table1_p = page_table1;

/* Kernel Functions */
int SetKernelBrk(void *addr) {
  int kernel_heap_limit_index;
  int addr_index;
  int pages_needed;
  int i;
  int frame;

  printf("in set kernel brk\n");
  printf("address value: %p\n", addr);

  if (VM_ENABLED) {
    printf("vm has been enabled...\n");
    kernel_heap_limit_index = get_page_index(KERNEL_HEAP_LIMIT);
    addr_index = get_page_index(addr);
    pages_needed = addr_index - kernel_heap_limit_index;
    printf("pages needed: %i\n", pages_needed);
    // Need more memory
    if (pages_needed > 0) {
      if (len_free_frames() >= pages_needed) {
        printf("free pages: %i\n", len_free_frames());
        for (i = 0; i < pages_needed; i++) {
          frame = get_free_frame();
          (page_table1_p + kernel_heap_limit_index - TABLE1_OFFSET + i)->valid = PTE_VALID;
          (page_table1_p + kernel_heap_limit_index - TABLE1_OFFSET + i)->pfn = frame;
          (page_table1_p + kernel_heap_limit_index - TABLE1_OFFSET + i)->uprot = PROT_NONE;
          (page_table1_p + kernel_heap_limit_index - TABLE1_OFFSET + i)->kprot = (PROT_READ | PROT_WRITE);
        }
        KERNEL_HEAP_LIMIT = addr;
        WriteRegister( REG_TLB_FLUSH, TLB_FLUSH_1);
        printf("[debug]:finished allocating memory...\n");
        return 0;
      } else {
        for (i = 0; i < pages_needed; i++) {
          (page_table1_p + kernel_heap_limit_index - TABLE1_OFFSET - i)->valid = PTE_INVALID;
          set_frame((page_table1_p + kernel_heap_limit_index - TABLE1_OFFSET - i)->pfn, FRAME_FREE);
          (page_table1_p + kernel_heap_limit_index - TABLE1_OFFSET - i)->pfn = 0;
          (page_table1_p + kernel_heap_limit_index - TABLE1_OFFSET - i)->uprot = PROT_NONE;
          (page_table1_p + kernel_heap_limit_index - TABLE1_OFFSET - i)->kprot = PROT_NONE;
        }
        return ERROR;
      }
    }
    //get the current page which should be the kernerlbrk
    //get page of the addr we need to go to
    //if the current page is less than the neededpage we need to free some memory TODO
    //otherwise need to increase memory
    //check to see if there is enough memory
    //need to get the vpn address somehow in order to accurately update the pte
    //make sure to TLB_FLUSH_1
    //TODO
 } else {
   printf("vm not enabled...\n");
    // vm is disabled, just move brk pointer up
    KERNEL_HEAP_LIMIT = addr;
  }
  return 0;
}

// Interrupt kernel
void KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args) {

  //Possibly have a global pcb which represents the current active process

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
  /* Get kernel addresses  (region1)*/
  KERNEL_HEAP_LIMIT = orig_brk;

  /* Frames, tables, etc.*/
  initialize_frames(num_frames);
  page_table0_p = create_page_table();
  assert(page_table0_p != NULL);
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
  // Page Table 0
  page_table0_p = init_page_table0(page_table0_p);

  /* Debug region values*/
  printf(DIVIDER);
  printf("pmem: %u\n", pmem_size);
  printf("free frames: %i\n", len_free_frames());
  debug_frames();
  printf(DIVIDER);
  debug_page_table(page_table0_p, 1);
  debug_page_table(page_table1_p, 1);
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


  /* Enable virtual memory */
  dprintf("enabling virtual memory...", 1);
  WriteRegister( REG_VM_ENABLE, (RCS421RegVal) 1);
  VM_ENABLED = true;
  dprintf("enabled virtual memory...", 1);



  // Create init program
  /* Create pcb for init program */
  struct pcb *init_pcb;

  init_pcb = Create_pcb(NULL);
  printf("[debug]: created pcb...\n");
  dprintf("created pcb", 0);
  pcb_current = init_pcb;
  pcb_current->pc_next = frame->pc;
  pcb_current->sp_next = frame->sp;
  pcb_current->psr_next = frame->psr;
  pcb_current->frame = frame;
  //TODO: free existing page tables or make it so that create pcb
  //doesn't malloc for page tables
  pcb_current->page_table_p = page_table0_p;


  // Load init program
  dprintf("about to load program...", 0);
  if(LoadProgram("init", cmd_args, frame, &pcb_current) != 0) {
    unix_error("error loading program!");
  }

  debug_pcb(pcb_current);
  printf("finished loading program...\n");
  fflush(stdout);

  if (VM_ENABLED) {
    WriteRegister( REG_TLB_FLUSH, TLB_FLUSH_0);
  }
  /*// Create idle*/
  /*//create_idle_pcb();*/
  /*start_idle();*/

  dprintf("done starting kernel!", 0);
}

/* Context switches */
/*
 * Performs first context switch into idle
 */
SavedContext* initswitchfunction(SavedContext *ctxp, void *p1, void *p2){
  dprintf("in initswitchfunction...", 0);
  fflush(stdout);

  // Get page table
  struct pcb *p = (struct pcb *) p1;
  struct pte *page_table = (p->page_table_p);

  // Update current page table
  WriteRegister( REG_PTR0, (RCS421RegVal) page_table);

  if (VM_ENABLED) {
    dprintf("flusing region0...", 0);
    WriteRegister( REG_TLB_FLUSH, TLB_FLUSH_0);
  }

  return ctxp;
}

/*
 * Create the idle pcb process
 */
void create_idle_pcb(ExceptionStackFrame *frame) {
  dprintf("in create_idle_pcb...",  0);
  pcb_idle = Create_pcb(NULL);
  pcb_idle->frame = frame;

  // Get context
  dprintf("creating initial saved_context", 0);
  SavedContext *ctx; //nothing in ctx in beginiing
  ctx = (SavedContext *)malloc(sizeof(SavedContext));
  /*dprintf("about to context switch...", 0);*/
  /*ContextSwitch(switchfunc_nop, ctx, (void *)pcb_idle, (void *)pcb_idle);*/

  // Load idle program
  dprintf("about to load program...", 0);
  LoadProgram("idle", args_copy, frame, &pcb_idle);
}

/*
 * Start the idle process
 */
void start_idle(ExceptionStackFrame *frame) {
  dprintf("in start_idle...", 0);
  pcb_idle = Create_pcb(NULL);
  pcb_idle->pc_next = frame->pc;
  pcb_idle->sp_next = frame->sp;
  pcb_idle->psr_next = frame->psr;
  pcb_idle->frame = frame;
  pcb_current = pcb_idle;

  // Get context
  dprintf("creating initial saved_context", 0);
  SavedContext *ctx; //nothing in ctx in beginiing
  ctx = (SavedContext *)malloc(sizeof(SavedContext));
  /*dprintf("about to context switch...", 0);*/
  /*ContextSwitch(switchfunc_nop, ctx, NULL, NULL);*/

  dprintf("about to contextswitch...", 0);
  fflush(stdout);
  ContextSwitch(initswitchfunction, ctx, pcb_current, NULL);

  dprintf("about to load program...", 0);
  LoadProgram("idle", args_copy, frame, &pcb_current);
  dprintf("finished loading program...", 0);
}
