#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

#include "simpleutils.h"
#include "yalnix_interrupts.h"
#include "yalnix_mem.h"


SavedContext* initswitchfunction(SavedContext *ctxp, void *p1, void *p2);
SavedContext* forkswitchfunction(SavedContext *ctxp, void *p1, void *p2 );

/* Extern */
extern int LoadProgram(char *name, char **args, ExceptionStackFrame *frame);

/* Globals */
void *interrupt_vector_table[TRAP_VECTOR_SIZE];
struct pcb *pcb_current;
bool VM_ENABLED;

struct pte page_table1[NUM_PAGES];
struct pte *page_table1_p = page_table1;

/* Kernel Functions */
int SetKernelBrk(void *addr) {
  printf("in set kernel brk\n");
  printf("address value: %p\n", addr);

  if (VM_ENABLED) {
    //get the current page which should be the kernerlbrk
    //get page of the addr we need to go to
    //if the current page is less than the neededpage we need to free some memory
    //otherwise need to increase memory
    //check to see if there is enough memory
    //need to get the vpn address somehow in order to accurately update the pte
    //make sure to TLB_FLUSH_1
    //TODO
 } else {
    // vm is disabled, just move brk pointer up
    KERNEL_HEAP_LIMIT = addr;
  }
  return 0;
}

/*
 * Idle process
 */
void process_idle() {
  for (;;){
    Pause();
    printf("idling...\n");
  }
}

// Interrupt kernel
void KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args) {

  //Possibly have a global pcb which represents the current active process

  /* Init vars */
  int i;
  int num_frames;
  int kernel_heap_limit_index;
  int kernel_text_limit_index;
  int kernel_stack_base_index;
  int kernel_stack_limit_index;

  /* Get memory size*/
  num_frames = pmem_size / PAGESIZE;
  assert(num_frames > 0); // silly...
  /* Get kernel addresses */
  KERNEL_HEAP_LIMIT = orig_brk;


  /* Frames and Tables*/
  initialize_frames(num_frames);
  page_table0_p = create_page_table();
  assert(page_table0_p != NULL);

  /* Initialize interrupt vector table */
  interrupt_vector_table[TRAP_KERNEL] = &interrupt_kernel;
  interrupt_vector_table[TRAP_CLOCK] = &interrupt_clock;
  interrupt_vector_table[TRAP_ILLEGAL] = &interrupt_illegal;
  interrupt_vector_table[TRAP_MEMORY] = &interrupt_memory;
  interrupt_vector_table[TRAP_MATH] = &interrupt_kernel;
  interrupt_vector_table[TRAP_TTY_RECEIVE] = &interrupt_kernel;
  interrupt_vector_table[TRAP_TTY_TRANSMIT] = &interrupt_kernel;
  interrupt_vector_table[TRAP_DISK] = &interrupt_kernel;
  for (i=8; i < TRAP_VECTOR_SIZE; i++) {
    interrupt_vector_table[i] = NULL;
  }

  /* Point REG_VECTOR_BASE at interupt vector table */
  WriteRegister( REG_VECTOR_BASE, (RCS421RegVal) &interrupt_vector_table);

  /* Initialize all page table entries to invalid */
  for (i=0; i< NUM_PAGES; i++) {
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
   *
   * User Stack LIMIT
   * ...
   * MEM_INVALID
   * VMEM_0_BASE (0)
   */

  kernel_heap_limit_index = get_page_index(KERNEL_HEAP_LIMIT);
  kernel_text_limit_index = get_page_index(&_etext);
  kernel_stack_limit_index = get_page_index(KERNEL_STACK_LIMIT);
  kernel_stack_base_index = get_page_index(KERNEL_STACK_BASE);
  printf("orig_brk: %p\n", orig_brk);
  printf("text: %p\n", &_etext);

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
  for(i=kernel_stack_base_index; i <= kernel_stack_limit_index; i++) {
    (page_table0_p + i)->valid = PTE_VALID;
    (page_table0_p + i)->pfn = i;
    (page_table0_p + i)->kprot = (PROT_READ | PROT_WRITE);
    (page_table0_p + i)->uprot = PROT_NONE;
    set_frame(i, FRAME_NOT_FREE);
  }

  /* Debug region values*/
  printf(DIVIDER);
  printf("pmem: %u\n", pmem_size);
  printf("free frames: %i\n", len_free_frames());
  debug_frames(0);
  printf(DIVIDER);
  debug_page_tables(page_table0_p, 1);
  debug_page_tables(page_table1_p, 1);
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
  WriteRegister( REG_VM_ENABLE, (RCS421RegVal) 1);
  VM_ENABLED = true;

  struct pte *idle_table;
  struct pcb *idle_pcb;

  idle_table = create_page_table(idle_table);
  if (idle_table == NULL) {
    printf("error creating page table\n");
    exit(1);
  }
  idle_pcb = create_pcb(NULL, *idle_table);
  if (idle_pcb == NULL) {
    printf("error creating pcb\n");
    exit(1);
  }
  pcb_current = idle_pcb;
  // Get contents of current pcb
  SavedContext *ctx = &(pcb_current->context);

  if(ContextSwitch(initswitchfunction, ctx, idle_pcb, NULL) == -1){
    printf("error with initswitching \n");
    exit(1);
  }
  if(LoadProgram("idle", cmd_args, frame) != 0) {
    //error error error
  }
  idle_pcb->pc_next = frame->pc;
  idle_pcb->sp_next = frame->sp;
  idle_pcb->psr_next = frame->psr;
  idle_pcb->frame = frame;

  printf("done!");
}

/*
 * Performs first context switch into idle
 */
SavedContext* initswitchfunction(SavedContext *ctxp, void *p1, void *p2){
  struct pcb *p = (struct pcb *) p1;
  struct pte *page = &(p->page_table);
  clone_page_table(page_table0_p, page);
  WriteRegister( REG_PTR0, (RCS421RegVal) page);
  if (VM_ENABLED) {
    WriteRegister( REG_TLB_FLUSH, TLB_FLUSH_0);
  }
  return ctxp;
}

/*
 * Context switch from fork
 */
SavedContext* forkswitchfunction(SavedContext *ctxp, void *p1, void *p2 ){
  struct pcb *parent = p1;
  struct pcb *child = p2;
  struct pte *parent_table = &(parent->page_table);
  struct pte *child_table = &(child->page_table);
  clone_page_table(parent_table, child_table);
  return ctxp;
}
