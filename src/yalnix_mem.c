#include "yalnix_mem.h"


extern void start_idle();

/* Globals */
int PID = 0;

/* Debug functions */
void
debug_page_table(struct pte *table, int verbosity) {
  int i;
  // if verbosity set, dump out contents of entire page talbe
  if (verbosity) {
    for (i=0; i<PAGE_TABLE_LEN; i++) {
      printf("mem: %p, i:%i, pfn: %u, valid: %u, uprot: %x,  kprot: %x\n", table + i, i, (table + i)->pfn, (table + i)->valid, (table + i)->uprot, (table + i)->kprot);
    }
  } // end verbosity
  printf("start address: %p\n", table);
  printf("end address: %p\n", table + PAGE_TABLE_LEN - 1);
}

// Debug stack frame
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

/* Debug physical frames */
void
debug_frames(int verbosity) {
	int i;
	printf("lim address: %p\n", frames_p + NUM_FRAMES);
	printf("base address: %p\n", frames_p);
	printf("free frames: %i\n", len_free_frames());
	printf("num frames: %i\n", NUM_FRAMES);
	if (verbosity) {
		for(i=0; i<NUM_FRAMES; i++) {
			printf("address: %p ", frames_p + i);
			if ((frames_p + i)->free == FRAME_FREE) {
				printf("mem free\n");
			} else if ((frames_p + i)->free == FRAME_NOT_FREE) {
				printf("mem not free\n");
			} else {
				printf("invalid frame entry: %i\n",(frames_p + i)->free);
			}
		}
	}// end verbosity
	printf("=============\n");
}

void debug_pcb(struct pcb *pcb_p) {
  printf("dumping pcb...\n");
  printf("pid: %i\n", pcb_p->pid);
  printf("brk index: %i\n", pcb_p->brk_index);
  printf("stack limit index: %i\n", pcb_p->stack_limit_index);
  printf("name: %s\n", pcb_p->name);
/*struct pcb{*/
  /*unsigned int pid;*/
  /*unsigned int time_current;*/
  /*unsigned int time_delay; // time to wait before process is restarted*/
  /*int status; // delayed, sleeping...*/
  /*void *brk; //heap limit DEPRECIATE?*/
  /*int brk_index; //heap limit*/
  /*void *stack_limit; // DEPRECIATE?*/
  /*int stack_limit_index;*/
  /*SavedContext *context;*/
  /*struct pte page_table[PAGE_TABLE_LEN];*/
  /*struct pcb *parent;*/
  /*struct pcb *children_active[5];*/
  /*struct pcb *children_wait[5];*/
  /*ExceptionStackFrame *frame;*/
  /*void *pc_next;*/
  /*void *sp_next;*/
  /*unsigned long psr_next;*/
  /*char *name; //for debugging purposes*/
}

/* Misc Functions */
int get_next_pid() {
  return ++PID;
}

/* Frame Functions */
/*
 * Initialize all frames to free.
 */
int
initialize_frames(int num_frames) {
	NUM_FRAMES = num_frames;
	int i;
	//page_frames frames[NUM_FRAMES];
	frames_p = (page_frames *)malloc( sizeof(page_frames) * NUM_FRAMES );
	// Error mallocing memory
	if (frames_p == NULL) {
		return -1;
	}
	//*frames_p = page_frames frames[NUM_FRAMES];
  for (i=0; i<NUM_FRAMES; i++) {
		(*(frames_p + i)).free = FRAME_FREE;
  }
	return 1;
}

/* Set frame free at given index */
int
set_frame(int index, int status) {
	(*(frames_p + index)).free = status;
	return 1;
}

/* Returns number of free frames in system */
int
len_free_frames() {
	int i;
	int count_free = 0;
	for (i=0; i<NUM_FRAMES; i++) {
		if ((*(frames_p + i)).free == FRAME_FREE) {
			count_free++;
		}
	}
	return count_free;
}

/*
 * We the index of the next available free farme.
 * -1 if there are no free frames
 */
int get_free_frame() {
  int i;
	for (i=0; i<NUM_FRAMES; i++) {
		if ((*(frames_p + i)).free == FRAME_FREE) {
      set_frame(i, FRAME_NOT_FREE);
      return i;
		}
	}
  return -1;
}

/*
 * Create a page table with no valid page tables
 */
struct pte *create_page_table() {
  /*struct pte page_table[PAGE_TABLE_LEN];*/
  struct pte *page_table = (struct pte *)malloc(sizeof(struct pte) * PAGE_TABLE_LEN);
  if (page_table == NULL) {
    return NULL;
  }
  if (0 > reset_page_table(page_table)) {
    return NULL;
  }
  return page_table;
}
/*
 * Set region 0 kernel stack to valid
 */
struct pte *init_page_table0(struct pte *page_table) {
  int i;
  int limit = get_page_index(KERNEL_STACK_LIMIT);
  int base = get_page_index(KERNEL_STACK_BASE);
  for(i=base; i<=limit; i++) {
    (page_table + i)->valid = PTE_VALID;
    (page_table + i)->pfn = i;
    (page_table + i)->kprot = (PROT_READ | PROT_WRITE);
    (page_table + i)->uprot = PROT_NONE;
    set_frame(i, FRAME_NOT_FREE);
  }
  return page_table;
}

/*
 * Clone pt2 into pt1
 */
struct pte *clone_page_table(struct pte *src) {
  int i;
  struct pte *dest = create_page_table();
  for (i=0; i<PAGE_TABLE_LEN; i++) {
    (dest + i)->pfn = (src + i)->pfn;
    (dest + i)->valid = (src +i)->valid;
    (dest + i)->uprot = (src +i)->uprot;
    (dest + i)->kprot = (src +i)->kprot;
  }
  return dest;
}

/*
 * Set's everything to invalid
 * NOTES: tested
 */
struct pte *reset_page_table(struct pte *page_table) {
  dprintf("in reset_page_table", 0);
  int i;
  for (i=0; i<PAGE_TABLE_LEN; i++) {
    (page_table + i)->pfn = PFN_INVALID;
    (page_table + i)->valid = PTE_INVALID;
    (page_table + i)->uprot = PROT_NONE;
    (page_table + i)->kprot = PROT_NONE;
  }
  return page_table;
}

/*
 * Reset page tables but keep kernel heap
 * Set all frames pointed to by page table to free
 */
struct pte *reset_page_table_limited(struct pte *page_table) {
  int i;
  for (i=0; i<get_page_index(KERNEL_STACK_BASE); i++) {
    if ((page_table + i)->valid == PTE_VALID) {
      set_frame((page_table + i)->pfn, FRAME_FREE);
    }
    (page_table + i)->pfn = PFN_INVALID;
    (page_table + i)->valid = PTE_INVALID;
    (page_table + i)->uprot = PROT_NONE;
    (page_table + i)->kprot = PROT_NONE;
  }
  return page_table;
}

/*
 * Malloc pcb space
 */
struct pcb *create_pcb(struct pcb *parent) {
  struct pcb *pcb_p;
  /*struct pte *page_table;*/

  // Initiate pcb
  pcb_p = (struct pcb *)malloc(sizeof(struct pcb));
  if (pcb_p == NULL) {
    unix_error("error creating pcb");
  }

  // Initiate page table
  /*page_table = create_page_table();*/
  /*if (NULL == page_table) unix_error("error creating page table!");*/
  /*page_table = init_page_table0(page_table);*/
  /*pcb_p->page_table_p = page_table;*/

  pcb_p->children_active = create_queue();
  pcb_p->children_wait = create_queue();
  if (parent != NULL) {
    //TODO: check to make sure pcb_p gets modifications
    enqueue(parent->children_active, pcb_p);
  }

  pcb_p->pid = get_next_pid(); //DETAIL (make sure this doesn't overflow...)
  pcb_p->time_current = 0;
  pcb_p->time_delay = 0;
  pcb_p->status = 0;
  pcb_p->parent = parent;
  pcb_p->frame = NULL;
  pcb_p->pc_next = NULL;
  pcb_p->sp_next = NULL;
  pcb_p->psr_next = -1; //TODO: what mode is this?
  return pcb_p;
}

/*
 * Free pcb structure
 */
int free_pcb(struct pcb *pcb_p) {
  /*free(pcb_p->page_table_p);*/
  free(pcb_p->children_wait);
  free(pcb_p->children_active); //TODO: free elemetns in queue
  free(pcb_p->context);
  free(pcb_p);
  return 1;
}

/*
 *
 */
struct pte *terminate_pcb(struct pcb *pcb_p) {
  dprintf("in terminate_pcb...", 0);
  struct pcb *p;
  elem *e;
  struct pte *page_table;

  page_table = pcb_current->page_table_p; // get reference to current page table
  pcb_current = NULL;  //doesn't exist any more

  // set parent of children to null - because they no longer have parents :(
  while(0 < pcb_p->children_active->len) {
    e = dequeue(pcb_p->children_active);
    p = (struct pcb *)e;
    p->parent = NULL;
  }
  dprintf("process has no active children...", 0);
  if (NULL == pcb_p->parent) {
    dprintf("process has no parent", 0);
  } else {
    dprintf("process has parent", 0);
  }
  printf("process status: %i\n", pcb_p->status);
  // zombie status need to be collected by parent
  pcb_p->status = STATUS_ZOMBIE;
  if (NULL != pcb_p->parent) {
    dprintf("process has a parent...", 0);
    enqueue(pcb_p->parent->children_wait, pcb_p);
    if (STATUS_WAIT == pcb_p->parent->status) {
      //TODO: remove parent from waiting queue
      // add removed parent to the ready queue
    }
  } else {
    dprintf("no parent found...", 0);
    free_pcb(pcb_p);
  }
  dprintf("exiting terminate_pcb...", 0);
  return page_table;
}

struct pcb *Create_pcb(struct pcb *parent) {
  struct pcb *a_pcb;
  a_pcb = create_pcb(parent);
  if (a_pcb == NULL) {
    unix_error("error creating pcb!");
  }
  return a_pcb;
}

/* Switching functions */

/*
 * Context switch from fork
 */
SavedContext* switchfunc_fork(SavedContext *ctxp, void *p1, void *p2 ){
  struct pcb *parent = p1;
  struct pcb *child = p2;
  struct pte *parent_table = (parent->page_table_p);
  struct pte *child_table = (child->page_table_p);
  child_table = clone_page_table(parent_table);
  return ctxp;
}

/*
 * Idle switch
 */
SavedContext* switchfunc_idle(SavedContext *ctxp, void *p1, void *p2){
  dprintf("in switchfunc_idle", 0);
  fflush(stdout);

  // Init function gone, there's no function to switch from
  struct pcb *p = (struct pcb *) p1;
  struct pte *page_table = (p->page_table_p);

  printf("[debug]: page 508: %u\n", (page_table + 508)->valid);
  dprintf("updating reg_ptr0...", 0);
  WriteRegister( REG_PTR0, (RCS421RegVal) page_table);

  /*if (VM_ENABLED) {*/
    /*dprintf("flusing region0...", 0);*/
    /*WriteRegister( REG_TLB_FLUSH, TLB_FLUSH_0);*/
  /*}*/

  return ctxp;
}

/*
 * Initial context switch into first process (idle)
 */
SavedContext* switchfunc_init(SavedContext *ctxp, void *p1, void *p2){
  dprintf("in switchfunc_idle...", 0);
  fflush(stdout);

  // Get page table
  struct pcb *p = (struct pcb *) p1;
  struct pte *page_table = (p->page_table_p);

  // Update register
  WriteRegister( REG_PTR0, (RCS421RegVal) page_table);

  // flush tlb
  if (VM_ENABLED) {
    WriteRegister( REG_TLB_FLUSH, TLB_FLUSH_0);
  }
  return ctxp;
}

/*
 * Returns context and nothign else
 */
SavedContext* switchfunc_nop(SavedContext *ctxp, void *p1, void *p2 ) {
  dprintf("in switchfunc_nop", 0);
  return ctxp;
}

/*
 * Switch from process 1 to process 2
 */
SavedContext *switchfunc_normal(SavedContext *ctxp, void *pcb1, void *pcb2) {
  dprintf("in switchfunc_normal...", 0);
  //struct pcb *p1 = (struct pcb *)p1;
  struct pcb *p2 = (struct pcb *)pcb2;
  //debug_page_table((struct pte *)ReadRegister(REG_PTR1), 1); //TODO:tmp

  // Set regs & psr
  /*p2->frame->pc = p2->pc_next;*/
  /*p2->frame->sp = p2->sp_next;*/
  /*p2->frame->psr = p2->psr_next;*/

  // Set pcb to new process
  pcb_current = p2;
  debug_pcb(p2);
  /*// Put p1 into queue*/
  /*enqueue(p_delay, (void *)p1);*/
  // switch region0 table to p2

  dprintf("about to write new page table address", 0);
  WriteRegister( REG_PTR0, (RCS421RegVal) p2->page_table_p);
  dprintf("about to flush..", 0);
  WriteRegister( REG_TLB_FLUSH, TLB_FLUSH_0);

  return ctxp;
}

/*
 * Called to context switch into first program
 * Sets the current page table to be the page table of p1
 * @params:
 * ctxp - pointer to context of p1
 * p1 - pointer to pcb of first process
 * p2 - should always be NULL (DEPRECIATE(?))
 * @return:
 * ctxp - ctx of p2
 */
SavedContext* initswitchfunction(SavedContext *ctxp, void *p1, void *p2){
  dprintf("in initswitchfunction...", 0);
  fflush(stdout);

  // Get page table
  struct pcb *p = (struct pcb *) p1;
  struct pte *page_table = (p->page_table_p);

  // debug
  debug_pcb(p);
  debug_page_table(page_table, 1);
  // Update current page table
  dprintf("writing ptr0 reg...", 0);
  WriteRegister( REG_PTR0, (RCS421RegVal) page_table);

  // flush out old entries
  if (VM_ENABLED) {
    dprintf("flusing region0...", 0);
    WriteRegister( REG_TLB_FLUSH, TLB_FLUSH_0);
  }
  return ctxp;
}


/*
 * Gets a process from ready or idle process
 */
void get_next_ready_process(struct pte *page_table) {
  dprintf("in get next ready process...", 0);
  elem *e;
  e = dequeue(p_ready);
  // no ready process, switch to idle
  if (NULL == e) {
    dprintf("switching idle...", 0);
    // current process free'd
    if (NULL == pcb_current) {
      dprintf("current proccess no longer exists", 0);
      debug_pcb(pcb_idle);
      // context switch into idle
      if (0 > ContextSwitch(switchfunc_normal, pcb_idle->context, NULL, pcb_idle)) {
        unix_error("failed context switch");
      }
      // page table of old process can now safely be free'd
      dprintf("freeing old page table...", 0);
      free(page_table);
    } else {
      // switch current process
      if (0 > ContextSwitch(switchfunc_normal, pcb_current->context, pcb_current, pcb_idle)) {
        unix_error("failed context switch");
      }
    }
    dprintf("finished switching...", 0);
    pcb_current = pcb_idle;
  } else {
    // switch to process in ready
    if (0 > ContextSwitch(switchfunc_normal, pcb_current->context, pcb_current, e->value)) {
      unix_error("failed context switch");
    }
    pcb_current = (struct pcb *)e->value;
  }
  dprintf("exit terminate pcb", 0);
}
