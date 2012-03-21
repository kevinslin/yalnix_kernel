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

#define PAGEMASK_REVERSE 0x3ff
#define PAGEMASK3 0x3ffffffff
#define EPHERMAL_BUFFER VMEM_1_SIZE - PAGESIZE

#define get_page_index(mem_address) (( ((long) mem_address) & PAGEMASK) >> PAGESHIFT)
//#define get_page_mem(page_index) ((void *)(page_index * PAGESIZE))
#define get_page_mem(page_index)  (((long) page_index & (long) PAGEMASK_REVERSE) << PAGESHIFT)

/* Structs */
typedef struct {
  int free;
} page_frames;

struct pcb{
  unsigned int pid;
  unsigned int time_current;
  unsigned int time_delay; // time to wait before process is restarted
  int status; // delayed, sleeping...
  int brk_index; //heap limit
  int stack_limit_index;
  SavedContext context;
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

  void *brk; //heap limit DEPRECIATE?
  void *stack_limit; // DEPRECIATE?
};

typedef struct _stream {
  int length;
  void *buf;
} stream;

/* Queues */
queue *p_ready;
queue *p_waiting;
queue *p_delay;

queue tty_read[NUM_TERMINALS];
queue tty_write[NUM_TERMINALS];
queue tty_read_wait[NUM_TERMINALS];
queue tty_write_wait[NUM_TERMINALS];
int tty_busy[NUM_TERMINALS];

/* Etc */
bool VM_ENABLED;
long page_brk;
struct pte *page_table0_p; // points to current region0 table
struct pte *page_table0_kernel; // points to current region0 table
bool IDLE_CREATED;
SavedContext *ctx_idle;
SavedContext *ctx_tmp;
ExceptionStackFrame *frame_idle;
char **cmd_args_idle;
ExceptionStackFrame *frame_current;
char **args_copy;


/* Pcb stuff */
struct pcb *pcb_current;
struct pcb *pcb_idle;

/* Frame Stuff */
int NUM_FRAMES; /* number of frames, obtained by dividing pmem_size / pagesize */
page_frames *frames_p;
void *KERNEL_HEAP_LIMIT;


/*######### Function Prototypes #########*/
/* Debug functions*/
void debug_page_table(struct pte *table, int verbosity);
void debug_stack_frame(ExceptionStackFrame *frame);
void debug_frames();
void debug_pcb(struct pcb *pcb_p);
void debug_process_queues();
void debug_tty_queues(int i);

/* Frame Functions */
int initialize_frames(int num_frames);
int set_frame(int index, int status);
int len_free_frames();
int get_free_frame();

/* Page Functions */
struct pte *create_page_table();
struct pte *create_page_table_helper(void * brk);
struct pte *init_page_table0(struct pte *page_table);
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
SavedContext* switchfunc_init(SavedContext *ctxp, void *p1, void *p2 );
SavedContext* switchfunc_normal(SavedContext *ctxp, void *p1, void *p2 );
SavedContext* initswitchfunction(SavedContext *ctxp, void *p1, void *p2); //DEPRECIATE!

/* Misc Functions */
int get_next_pid();
void get_next_ready_process(struct pte *page_table);
void extract_page_table(struct pte *page_table_dst, struct pte *page_table_src);
void clone_page_table_alt(struct pte *page_table_dst, struct pte *page_table_src);


#endif	/* end _yalnix_mem_h */
