#ifndef _yalnix_mem_h
#define _yalnix_mem_h

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <comp421/hardware.h>
#include "queue.h"
#include "simpleutils.h"

#define PTE_VALID 1
#define PTE_INVALID 0
#define PFN_INVALID  0xdeadb

#define TABLE1_OFFSET 512
#define FRAME_NOT_FREE -1
#define FRAME_FREE 1

#define MAX_CHILDREN 5

#define STATUS_NONE 0
#define STATUS_RUNNING 1
#define STATUS_WAIT 2
#define STATUS_ZOMBIE 3

#define TTY_FREE 0
#define TTY_BUSY 1

#define get_page_index(mem_address) (((long) mem_address & PAGEMASK) >> PAGESHIFT)
#define get_page_mem(page_index) ((void *)(page_index * PAGESIZE))

/* Structs */
typedef struct {
  int free;
} page_frames;

struct pcb{
  unsigned int pid;
  unsigned int time_current;
  unsigned int time_delay; // time to wait before process is restarted
  int status; // delayed, sleeping...
  void *brk; //heap limit DEPRECIATE?
  int brk_index; //heap limit
  void *stack_limit; // DEPRECIATE?
  int stack_limit_index;
  SavedContext *context;
  struct pte *page_table_p;
  struct pte *page_table_p_physical;
  struct pcb *parent;
  queue *children_active;
  queue *children_wait;
  ExceptionStackFrame *frame;
  void *pc_next;
  void *sp_next;
  unsigned long psr_next;
  char *name; //for debugging purposes
};

typedef struct _stream {
  int length;
  void *buf;
} stream;

/* Queues */
queue *p_ready;
queue *p_waiting;
queue *p_delay;

queue *tty_read[NUM_TERMINALS];
queue *tty_write[NUM_TERMINALS];
queue *tty_read_wait[NUM_TERMINALS];
queue *tty_write_wait[NUM_TERMINALS];
int tty_busy[NUM_TERMINALS];

/* Etc */
bool VM_ENABLED;



/* Pcb stuff */
struct pcb *pcb_current;
struct pcb *pcb_idle;

/* Frame Stuff */
int NUM_FRAMES; /* number of frames, obtained by dividing pmem_size / pagesize */
page_frames *frames_p;
void *KERNEL_HEAP_LIMIT;

/* Page table stuff */
//struct pte *page_table0_p; // the current pagetable0

/*######### Function Prototypes #########*/
/* Debug functions*/
void debug_page_table(struct pte *table, int verbosity);
void debug_stack_frame(ExceptionStackFrame *frame);
void debug_frames();
void debug_pcb(struct pcb *pcb_p);

/* Frame Functions */
int initialize_frames(int num_frames);
int set_frame(int index, int status);
int len_free_frames();
int get_free_frame();

/* Page Functions */
struct pte *create_page_table(struct pcb *p);
struct pte *init_page_table0(struct pte *page_table);
struct pte *clone_page_table(struct pte *src, struct pte **dst);
struct pte *reset_page_table(struct pte *page_table);
struct pte *reset_page_table_limited(struct pte *page_table);
int free_page_table(struct pte *page_table);

/* PCB functions */
struct pcb *Create_pcb(struct pcb *parent);
struct pcb *create_pcb(struct pcb *parent);
int free_pcb(struct pcb *pcb_p);
struct pte *terminate_pcb(struct pcb *pcb_p);

/* Terminals */
void initialize_terminals();

/* Context switch funcs */
SavedContext* switchfunc_fork(SavedContext *ctxp, void *p1, void *p2 );
SavedContext* switchfunc_idle(SavedContext *ctxp, void *p1, void *p2 );
SavedContext* switchfunc_nop(SavedContext *ctxp, void *p1, void *p2 );
SavedContext* switchfunc_normal(SavedContext *ctxp, void *p1, void *p2 );
SavedContext* initswitchfunction(SavedContext *ctxp, void *p1, void *p2);

/* Misc Functions */
int get_next_pid();
void get_next_ready_process(struct pte *page_table);


#endif	/* end _yalnix_mem_h */
