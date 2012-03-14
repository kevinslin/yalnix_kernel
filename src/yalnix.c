#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

//#include "load.c"
#include "utils.h"
#include "yalnix_mem.c"


#define get_page_index(mem_address) (((long) mem_address & PAGEMASK) >> PAGESHIFT)
#define NUM_PAGES 512



/*
 * Idle process
 */
void process_idle() {
  for (;;){
    Pause();
    printf("idling...\n");
  }
}

/* Diagnostics */
void
debug_page_tables(struct pte *table, int verbosity) {
  int i;
  if (verbosity) {
    for (i=0; i<NUM_PAGES; i++) {
      printf("i:%i, pfn: %u, valid: %u\n", i, (table + i)->pfn, (table + i)->valid);
    }
  } // end verbosity
}

void
debug_stack_frame(ExceptionStackFrame *frame) {
  printf("vector: %i\n", frame->vector); // type of interrupt
  printf("code: %i\n", frame->code); // additional info for interrupt
  printf("processor status register: %lu\n", frame->psr); /*if (psr & PSR_MODE), then kernel mode */
  printf("address: %p\n", frame->addr); /* contains memory address of TRAP_MEMORY Exception */
  printf("pc: %p\n", frame->pc); /* pc at time of interrupt */
  printf("sp: %p\n", frame->sp); /* stack pointer value at time of interrupt */
  printf(DIVIDER);
}

void
interrupt_kernel(ExceptionStackFrame *frame) {
  printf("got a kernel interrupt\n");
  debug_stack_frame(frame);
}

void KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args) {
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

  /* Frames and Tables*/
  /*initialize_frames(num_frames);*/
  struct pte *page_table0_p = (struct pte *)malloc(sizeof(struct pte) * NUM_PAGES);
  struct pte *page_table1_p = (struct pte *)malloc(sizeof(struct pte) * NUM_PAGES);
  debug_frames(0);

  /* TODO: Initialize interrupt vector table */
  interrupt_vector_table[TRAP_KERNEL] = &interrupt_kernel;
  for (i=1; i < TRAP_VECTOR_SIZE; i++) {
    interrupt_vector_table[i] = NULL;
  }

  /* Point REG_VECTOR_BASE at interupt vector table */
  WriteRegister( REG_VECTOR_BASE, (RCS421RegVal) &interrupt_vector_table);

  /* Initialize all page table entries to invalid */
  for (i=0; i< NUM_PAGES; i++) {
    // Region 0
    (page_table0_p + i)->pfn = PFN_INVALID;
    (page_table0_p + i)->valid = PTE_INVALID;
    (page_table0_p + i)->uprot = PROT_NONE;
    (page_table0_p + i)->kprot = PROT_NONE;
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
  kernel_heap_limit_index = get_page_index(orig_brk);
  kernel_text_limit_index = get_page_index(&_etext);
  kernel_stack_limit_index = get_page_index(KERNEL_STACK_LIMIT);
  kernel_stack_base_index = get_page_index(KERNEL_STACK_BASE);

  /* Map physical pages to corresponding virtual addresses */
  // TABLE1_OFFSET neccessary to account for VMEM_1_BASE higher starting address
  for(i=kernel_text_limit_index; i <= kernel_heap_limit_index + 1; i++) {
    (page_table1_p + i - TABLE1_OFFSET)->valid = PTE_VALID;
    (page_table1_p + i - TABLE1_OFFSET)->pfn = i;
    (page_table1_p + i - TABLE1_OFFSET)->kprot = (PROT_READ | PROT_WRITE);
    /*set_frame(i, FRAME_NOT_FREE);*/
  }
  for(i=get_page_index(VMEM_1_BASE); i < kernel_text_limit_index; i++) {
    (page_table1_p + i - TABLE1_OFFSET)->valid = PTE_VALID;
    (page_table1_p + i - TABLE1_OFFSET)->pfn = i;
    (page_table1_p + i - TABLE1_OFFSET)->kprot = (PROT_READ | PROT_EXEC);
    /*set_frame(i, FRAME_NOT_FREE);*/
  }
  // Page Table 0
  for(i=kernel_stack_base_index; i <= kernel_stack_limit_index; i++) {
    (page_table0_p + i)->valid = PTE_VALID;
    (page_table0_p + i)->pfn = i;
    (page_table0_p + i)->kprot = (PROT_READ | PROT_WRITE);
    (page_table0_p + i)->uprot = PROT_NONE;
    /*set_frame(i, FRAME_NOT_FREE);*/
  }

  printf("pmem: %u\n", pmem_size);
  printf("free frames: %i\n", len_free_frames());
  debug_frames(0);
  debug_page_tables(page_table0_p, 1);
  debug_page_tables(page_table1_p, 1);

  WriteRegister( REG_PTR0, (RCS421RegVal) page_table0_p);
  WriteRegister( REG_PTR1, (RCS421RegVal) page_table1_p);

  /* Enable virtual memory */
  WriteRegister( REG_VM_ENABLE, (RCS421RegVal) 1);

  /* Create an idle process */
  process_idle();

  // Test
  printf("hello world!");
}

int SetKernelBrk(void *size) {
  return 0;
}

