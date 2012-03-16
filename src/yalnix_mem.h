#ifndef _yalnix_mem_h
#define _yalnix_mem_h

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <comp421/hardware.h>
#include "simpleutils.h"

#define PTE_VALID 1
#define PTE_INVALID 0
#define PFN_INVALID  0xdeadb
#define NUM_PAGES 512

#define TABLE1_OFFSET 512
#define FRAME_NOT_FREE -1
#define FRAME_FREE 1

#define MAX_CHILDREN 5

#define get_page_index(mem_address) (((long) mem_address & PAGEMASK) >> PAGESHIFT)

typedef struct {
  int free;
} page_frames;

struct pcb{
  unsigned int pid;
  unsigned int time_current;
  unsigned int time_delay; // time to wait before process is restarted
  int status; // delayed, sleeping...
  void *brk; //heap limit
  void *stack_limit;
  SavedContext context;
  struct pte page_table;
  struct pcb *parent;
  struct pcb *children_active[5];
  struct pcb *children_wait[5];
  ExceptionStackFrame *frame;
  void *pc_next;
  void *sp_next;
  unsigned long psr_next;
};

/* Frame Stuff */
int NUM_FRAMES; /* number of frames, obtained by dividing pmem_size / pagesize */
page_frames *frames_p;
void *KERNEL_HEAP_LIMIT;
/* Page table stuff */
struct pte *page_table0_p;
/* Misc Functions */
int get_next_pid();

/* Frame Functions */
int initialize_frames(int num_frames);
int set_frame(int index, int status);
int len_free_frames();
int get_free_frame();
/* Page Functions */
struct pte *create_page_table();
struct pte *clone_page_table(struct pte *src);
struct pte *reset_page_table(struct pte *page_table);
struct pte *reset_page_table_limited(struct pte *page_table);
int free_page_table(struct pte *page_table);
/* PCB functions */
struct pcb *create_pcb(struct pcb *parent, struct pte page_table);
int free_pcb(struct pcb *pcb_p);

/* Debug functions*/
void debug_page_tables(struct pte *table, int verbosity);
void debug_stack_frame(ExceptionStackFrame *frame);
void debug_frames();

#endif	/* end _yalnix_mem_h */
