#include <stdio.h>
#include <stdlib.h>

#include <comp421/yalnix.h>
#include <comp421/hardware.h>

static int **interrupt_vector_table[TRAP_VECTOR_SIZE];
typedef struct {
  int dummy;
} free_page_frames;
typedef struct {
  int dummy;
} page_table;

void KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args) {
  /* Init */
  int i;
  /* Initialize interrupt vector table */
  for (i=0; i < TRAP_VECTOR_SIZE; i++) {
    interrupt_vector_table[i] = NULL;
  }
  /* Point REG_VECTOR_BASE at interupt vector table */
  WriteRegister( REG_VECTOR_BASE, (RCS421RegVal) &interrupt_vector_table);
  /* Keep track of free page frames */
  free_page_frames free_frames = { 1 };
  /* Initial page tables for region 0 and region 1 */
  page_table p_reg0;
  page_table p_reg1;
  WriteRegister( REG_PTR0, (RCS421RegVal) &p_reg0);
  WriteRegister( REG_PTR1, (RCS421RegVal) &p_reg1);
  /* Map existing physical addresses to same virtual addresses */
  /* Enable virtual memory */
  WriteRegister( REG_VM_ENABLE, (RCS421RegVal) 1);
  /* Create an idle process */
  for (;;){
    Pause();
  }

  // Test
  printf("hello world!");
}

int SetKernelBrk(void *size) {
  return 0;
}

