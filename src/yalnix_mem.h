#ifndef _yalnix_mem_h
#define _yalnix_mem_h

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <comp421/hardware.h>
#include "simpleutils.h"

#define PTE_VALID 1
#define PTE_INVALID 0
#define PFN_INVALID  -1
#define NUM_PAGES 512

#define TABLE1_OFFSET 512
#define FRAME_NOT_FREE -1
#define FRAME_FREE 1

#define get_page_index(mem_address) (((long) mem_address & PAGEMASK) >> PAGESHIFT)

typedef struct {
  int free;
} page_frames;
typedef struct {

} pcb;

/* Frame Stuff */
int NUM_FRAMES; /* number of frames, obtained by dividing pmem_size / pagesize */
page_frames *frames_p;
void *KERNEL_HEAP_LIMIT;
/* Page table stuff */
struct pte *page_table0_p;
struct pte page_table1[NUM_PAGES];
struct pte *page_table1_p = page_table1;



/* Frame Functions */
int initialize_frames(int num_frames);
int set_frame(int index, int status);
int len_free_frames();
int get_free_frame();
/* Page Functions */
int create_page_table(struct pte *page_table);
int reset_page_table(struct pte *page_table);
int reset_page_table_limited(struct pte *page_table);
int free_page_table(struct pte *page_table);

/* Debug functions*/
void debug_page_tables(struct pte *table, int verbosity);
void debug_stack_frame(ExceptionStackFrame *frame);
void debug_frames();

#endif	/* end _yalnix_mem_h */
