#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <comp421/yalnix.h>
#include <comp421/hardware.h>

static int **interrupt_vector_table[TRAP_VECTOR_SIZE];


typedef struct {
  int free;
} page_frames;



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
  /* Initialize constants */
  num_frames = pmem_size / PAGESIZE;
  assert(num_frames > 0); // silly...
  num_pages = VMEM_REGION_SIZE / PAGESIZE;
  assert(num_pages > 0); // silly...

  /* More constants */
  page_frames frames[num_frames];
  pte page_table1[num_pages];
  pte page_table2[num_pages];

  /* Initialize interrupt vector table */
  for (i=0; i < TRAP_VECTOR_SIZE; i++) {
    interrupt_vector_table[i] = NULL;
  }
  /* Point REG_VECTOR_BASE at interupt vector table */
  WriteRegister( REG_VECTOR_BASE, (RCS421RegVal) &interrupt_vector_table);
  /* Keep track of free page frames */

  for (i=0; i<num_frames; i++) {
    frames[i].free = 0;
  }
  /* Initial page tables for region 0 and region 1 */
  for (i=0; i< num_pages; i++) {
    page_table1[i].valid = 0;
    page_table1[i].pfn = 0
    page_table2[i].valid = 0;
    page_table2[i].frame_num = 0;
  }
  WriteRegister( REG_PTR0, (RCS421RegVal) &page_table1);
  WriteRegister( REG_PTR1, (RCS421RegVal) &page_table2);
  /* Map existing physical addresses to same virtual addresses */
  // TODO

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

