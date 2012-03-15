#include <stdbool.h>
/*
 * Handle page frames and page tables
 *
 */

#ifndef _yalnix_mem_h
#define _yalnix_mem_h

#define PTE_VALID 1
#define PTE_INVALID 0
#define PFN_INVALID  -1
#define NUM_PAGES 512

#define TABLE1_OFFSET 512

typedef struct {
  int free;
} page_frames;


/* Globals */
static void *interrupt_vector_table[TRAP_VECTOR_SIZE];
static page_frames *frames_p;

static bool VM_ENABLED = false;
static void *KERNEL_HEAP_LIMIT;

int NUM_FRAMES;

int initialize_frames(int num_frames);
int set_frame(int index, int status);
int len_free_frames();
void debug_frames();

struct pte page_table1[NUM_PAGES];
// depreciate?
struct pte *page_table1_p = page_table1;

#endif	/* end _yalnix_mem_h */
