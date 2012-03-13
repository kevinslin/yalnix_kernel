#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

//#include "load.c"
#include "yalnix_mem.c"


#define get_page_index(mem_address) (((long) mem_address & PAGEMASK) >> PAGESHIFT)


/*
 * Idle process
 */
void process_idle() {
  for (;;){
    Pause();
    printf("idling...\n");
  }
}

void KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args) {
  /* Init */
  int i;
  int num_frames;
  int num_pages;
  int kernel_heap_limit_index;
  int kernel_text_limit_index;
  int kernel_stack_base_index;
  int kernel_stack_limit_index;

  /* Initialize constants */
  num_frames = pmem_size / PAGESIZE;
  assert(num_frames > 0); // silly...
  num_pages = VMEM_REGION_SIZE / PAGESIZE;
  assert(num_pages > 0); // silly...

  /* Frames and Tables*/
  initialize_frames(num_frames);
  struct pte page_table0[num_pages];
  struct pte page_table1[num_pages];

  debug_frames();
  printf("base page table0: %p\n", page_table0);
  printf("base page table1: %p\n", page_table1);

  /* TODO: Initialize interrupt vector table */
  for (i=0; i < TRAP_VECTOR_SIZE; i++) {
    interrupt_vector_table[i] = NULL;
  }

  /* Point REG_VECTOR_BASE at interupt vector table */
  WriteRegister( REG_VECTOR_BASE, (RCS421RegVal) &interrupt_vector_table);

  /* Keep track of free page frames */

  /* Initial page tables for region 0 and region 1 */
  for (i=0; i< num_pages; i++) {
    // Region 0
    page_table0[i].pfn = PFN_INVALID;
    page_table0[i].valid = PTE_INVALID;
    page_table0[i].uprot = PROT_NONE;
    page_table0[i].kprot = PROT_NONE;
    // Region 1
    page_table1[i].pfn = PFN_INVALID;
    page_table1[i].valid = PTE_INVALID;
    page_table1[i].uprot = PROT_NONE;
    page_table1[i].kprot = PROT_NONE;
  }


  // Initialize the kernel pages
  kernel_heap_limit_index = get_page_index(orig_brk);
  kernel_text_limit_index = get_page_index(&_etext);
  kernel_stack_limit_index = get_page_index(KERNEL_STACK_LIMIT);
  kernel_stack_base_index = get_page_index(KERNEL_STACK_BASE);

  /*
   * ...
   * ...
   * KERNEL_HEAP_INDEX (593)
   * ...
   * KERNEL_TEXT_LIMIT (589)
   * ...
   * VMEM_1_BASE
   * ==================
   * Region 0
   * ====================
   * ...
   * KERNEL_STACK_LIMIT (512)
   * ...
   * KERNEL_STACK_BASE (508)
   * User Stack
   * ...
   * MEM_INVALID
   */

  // Page Table 1
  // TABLE1_OFFSET neccessary to account for VMEM_1_BASE higher starting address
  for(i=get_page_index(VMEM_1_BASE); i < kernel_text_limit_index; i++) {
    page_table1[i - TABLE1_OFFSET].valid = PTE_VALID;
    page_table1[i - TABLE1_OFFSET].pfn = i;
    page_table1[i - TABLE1_OFFSET].kprot = (PROT_READ | PROT_EXEC);
    //set_frame(i, FRAME_NOT_FREE);
  }
  for(i=kernel_text_limit_index; i <= kernel_heap_limit_index; i++) {
    page_table1[i - TABLE1_OFFSET].valid = PTE_VALID;
    page_table1[i - TABLE1_OFFSET].pfn = i;
    page_table1[i - TABLE1_OFFSET].kprot = (PROT_READ | PROT_WRITE);
    //set_frame(i, FRAME_NOT_FREE);
  }
  // Page Table 0
  for(i=kernel_stack_base_index; i <= kernel_stack_limit_index; i++) {
    page_table0[i].valid = PTE_VALID;
    page_table0[i].pfn = i;
    page_table0[i].kprot = (PROT_READ | PROT_WRITE);
    page_table0[i].uprot = PROT_NONE;
    //set_frame(i, FRAME_NOT_FREE);
  }

  printf("pmem: %u\n", pmem_size);
  printf("free frames: %i\n", len_free_frames());
  debug_frames();

  WriteRegister( REG_PTR0, (RCS421RegVal) &page_table0);
  WriteRegister( REG_PTR1, (RCS421RegVal) &page_table1);

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

