#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <comp421/yalnix.h>
#include <comp421/hardware.h>

#define PTE_VALID 1
#define PTE_INVALID -1
#define PFN_INVALID  -1

#define FRAME_NOT_FREE -1
#define FRAME_FREE 1
#define TABLE1_OFFSET 512

#define get_page_index(mem_address) (((long) mem_address & PAGEMASK) >> PAGESHIFT)


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
  int kernel_heap_limit_index;
  int kernel_text_limit_index;
  int kernel_stack_base_index;
  int kernel_stack_limit_index;
  int t1, t2;

  /* Initialize constants */
  num_frames = pmem_size / PAGESIZE;
  assert(num_frames > 0); // silly...
  num_pages = VMEM_REGION_SIZE / PAGESIZE;
  assert(num_pages > 0); // silly...

  /* Frames and Tables*/
  page_frames frames[num_frames];
  struct pte page_table0[num_pages];
  struct pte page_table1[num_pages];

  /* TODO: Initialize interrupt vector table */
  for (i=0; i < TRAP_VECTOR_SIZE; i++) {
    interrupt_vector_table[i] = NULL;
  }

  /* Point REG_VECTOR_BASE at interupt vector table */
  WriteRegister( REG_VECTOR_BASE, (RCS421RegVal) &interrupt_vector_table);

  /* Keep track of free page frames */
  for (i=0; i<num_frames; i++) {
    frames[i].free = FRAME_FREE;
  }

  /* Initial page tables for region 0 and region 1 */
  for (i=0; i< num_pages; i++) {
    page_table0[i].pfn = PFN_INVALID;
    page_table0[i].valid = PTE_INVALID;
    page_table0[i].uprot = PROT_NONE;
    page_table0[i].kprot = PROT_NONE;

    page_table1[i].pfn = PFN_INVALID;
    page_table1[i].valid = PTE_INVALID;
    page_table1[i].uprot = PROT_NONE;
    page_table1[i].kprot = PROT_NONE;
  }


  // Initialize the kernel pages
  kernel_heap_limit_index = get_page_index(orig_brk);
  kernel_text_limit_index = get_page_index(&_etext);
  kernel_stack_base_index = get_page_index(KERNEL_STACK_BASE);
  kernel_stack_limit_index = get_page_index(KERNEL_STACK_LIMIT);
  /*
   * ...
   * ...
   * KERNEL_HEAP_INDEX (593)
   * ...
   * KERNEL_TEXT_LIMIT (589)
   * ...
   * VMEM_1_BASE
   * ...
   * KERNEL_STACK_LIMIT (512)
   * ...
   * KERNEL_STACK_BASE (508)
   */

  // Page Table 1
  for(i=get_page_index(VMEM_1_BASE); i < kernel_text_limit_index; i++) {
    page_table1[i - TABLE1_OFFSET].valid = PTE_VALID;
    page_table1[i - TABLE1_OFFSET].pfn = i;
    page_table1[i - TABLE1_OFFSET].kprot = (PROT_READ | PROT_EXEC);
    frames[i].free = FRAME_NOT_FREE;
  }
  for(i=kernel_text_limit_index; i <= kernel_heap_limit_index; i++) {
    page_table1[i - TABLE1_OFFSET].valid = PTE_VALID;
    page_table1[i - TABLE1_OFFSET].pfn = i;
    page_table1[i - TABLE1_OFFSET].kprot = (PROT_READ | PROT_WRITE);
    frames[i].free = FRAME_NOT_FREE;
  }
  // Page Table 0
  for(i=kernel_stack_base_index; i <= kernel_stack_limit_index; i++) {
    page_table0[i].valid = PTE_VALID;
    page_table0[i].pfn = i;
    page_table0[i].kprot = (PROT_READ | PROT_WRITE);
    page_table0[i].uprot = PROT_NONE;
    frames[i].free = FRAME_NOT_FREE;
  }


  WriteRegister( REG_PTR0, (RCS421RegVal) &page_table0);
  WriteRegister( REG_PTR1, (RCS421RegVal) &page_table1);

  /* Map existing physical addresses to same virtual addresses */
  // Vector Table
  /*page_table1[get_page_index(interrupt_vector_table)].valid = PTE_VALID;*/
  /*page_table1[get_page_index(interrupt_vector_table)].pfn= get_page_index(interrupt_vector_table);*/
  /*// Page Frames*/
  /*t1 = get_page_index(&frames);*/
  /*page_table1[t1].valid = PTE_VALID;*/
  /*page_table1[t1].pfn = t1;*/
  /*// Page Tables*/
  /*t1 = get_page_index(&page_table0);*/
  /*t2 = get_page_index(&page_table1);*/
  /*printf("page table 0 index: %i;", t1);*/
  /*printf("page table 1 index: %i;", t2);*/

  /*page_table1[t1].valid = PTE_VALID;*/
  /*page_table1[t1].pfn = t1;*/
  /*page_table1[t2].valid = PTE_VALID;*/
  /*page_table1[t2].pfn = t2;*/

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

