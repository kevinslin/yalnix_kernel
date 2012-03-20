#include "yalnix_mem.h"


extern void start_idle();
extern struct pte *page_table1_p;

/* Globals */
// incremented every time a new process is started
int PID = 0;
struct pte pte_null = {0, 0, 0, 0, 0};

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
  printf("dumping physical frames...\n");
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
  printf(DIVIDER);
  printf("dumping pcb...\n");
  printf("pid: %i\n", pcb_p->pid);
  printf("brk index: %i\n", pcb_p->brk_index);
  printf("stack limit index: %i\n", pcb_p->stack_limit_index);
  printf("pte virtual: %p\n", pcb_p->page_table_p);
  printf("pte physical: %p\n", pcb_p->page_table_p_physical);
  printf("context: %p\n", pcb_p->context);
  printf("name: %s\n", pcb_p->name);
  printf(DIVIDER);
}

/*######### Frame Functions #########*/
/*
 * Malloc frames and set all to free
 */
int
initialize_frames(int num_frames) {
	int i;
	NUM_FRAMES = num_frames; // number of frames in our page talbe
  // malloc frames
	frames_p = (page_frames *)malloc( sizeof(page_frames) * NUM_FRAMES );
	if (frames_p == NULL) unix_error("can't malloc frames");

  // set all to free
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
int
get_free_frame() {
  int i;
	for (i=0; i<NUM_FRAMES; i++) {
		if ((*(frames_p + i)).free == FRAME_FREE) {
      set_frame(i, FRAME_NOT_FREE);
      return i;
		}
	}
  printf("did not find free frame!\n");
  return -1;
}


/*######### Page Tables  #########*/
/*
 * Create page table
 * The pagebrk starts off at vmem_1_limit.
 * Round it down and decrement to free up space for a page table.
 */
struct pte *create_page_table() {
  // find free frame for page table
  long old = DOWN_TO_PAGE(page_brk) / PAGESIZE;
  page_brk -= PAGESIZE;
  long new = DOWN_TO_PAGE(page_brk) / PAGESIZE;
  if (old == new) {
    return create_page_table_helper((void *)page_brk);
  }
  if ((frames_p + new)->free == FRAME_NOT_FREE) {
    unix_error("can't create page table");
  } else {
    set_frame(new, FRAME_FREE); // maybe redundant
    int offset = new - (VMEM_1_BASE / PAGESIZE); // index into pte1
    (page_table1_p + offset)->valid = PTE_VALID;
    (page_table1_p + offset)->pfn = new;
    (page_table1_p + offset)->kprot = (PROT_READ | PROT_WRITE);
    (page_table1_p + offset)->uprot = PROT_NONE;
    return create_page_table_helper((void *)page_brk);
  }
  return NULL;
}

struct pte *create_page_table_helper(void * brk) {
  struct pte *page_table;
  if (NULL == brk) {
    page_table = (struct pte *)malloc(PAGE_TABLE_SIZE);
    if (NULL == page_table) unix_error("can't malloc page table");
  } else {
    page_table = (struct pte *)brk;
  }
  reset_page_table(page_table);
  return page_table;
}

/*
 * Initialize a region0 page table
 * Set all entries to invalid except kernel stack
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
 * Mallocs room for new page table TODO: possible memory leak
 */
struct pte *clone_page_table(struct pte *src, struct pte **dest) {
  int i;
  for (i=0; i<PAGE_TABLE_LEN; i++) {
    (*dest + i)->pfn = (src + i)->pfn;
    (*dest + i)->valid = (src +i)->valid;
    (*dest + i)->uprot = (src +i)->uprot;
    (*dest + i)->kprot = (src +i)->kprot;
  }
  return *dest;
}

/*
 * Sets everything to invalid
 */
struct pte *reset_page_table(struct pte *page_table) {
  dprintf("in reset_page_table", 0);
  int i;
  for (i=0; i<PAGE_TABLE_LEN; i++) {
    *(page_table + i) = pte_null;
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
 * Like reset page table but keep the kernel stack
 * Set all frames pointed to by page table to free
 */
struct pte *reset_page_table_limited(struct pte *page_table) {
  int i;
  for (i=0; i<get_page_index(KERNEL_STACK_BASE); i++) {
    // if page was valid, free the frame
    if ((page_table + i)->valid == PTE_VALID) set_frame((page_table + i)->pfn, FRAME_FREE);
    (page_table + i)->pfn = PFN_INVALID;
    (page_table + i)->valid = PTE_INVALID;
    (page_table + i)->uprot = PROT_NONE;
    (page_table + i)->kprot = PROT_NONE;
  }
  return page_table;
}

/*######### PCB #########*/
/*
 * Create pcb
 */
struct pcb *create_pcb(struct pcb *parent) {
  struct pcb *pcb_p;

  // Initiate pcb
  pcb_p = (struct pcb *)malloc(sizeof(struct pcb));
  if (pcb_p == NULL) unix_error("error creating pcb");

  // Set children queues
  pcb_p->children_active = create_queue();
  pcb_p->children_wait = create_queue();
  if (parent != NULL) {
    //TODO: check to make sure pcb_p gets modifications
    enqueue(parent->children_active, pcb_p);
  }
  pcb_p->pid = get_next_pid();
  pcb_p->time_current = 0;
  pcb_p->time_delay = 0;
  pcb_p->status = STATUS_NONE;
  pcb_p->parent = parent;
  pcb_p->frame = NULL;
  pcb_p->pc_next = NULL;
  pcb_p->sp_next = NULL;
  pcb_p->psr_next = -1; //TODO: what mode is this?
  return pcb_p;
}

/*
 * Free pcb structure
 * Don't free page tables, they are free'd in terminate_page_table
 */
int free_pcb(struct pcb *pcb_p) {
  dprintf("freeing pcb...", 0);
  free(pcb_p->children_wait);
  free(pcb_p->children_active); //TODO: free elemetns in queue
  free(pcb_p->context);
  free(pcb_p);
  return 1;
}

/*
 * Detroy pcb and update book keeping information
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

/*######### Terminals #########*/

void initialize_terminals() {
  int i;
  for (i=0; i<NUM_TERMINALS; i++) {
    tty_read[i] = create_queue();
    tty_write[i] = create_queue();
    tty_read_wait[i] = create_queue();
    tty_write_wait[i] = create_queue();
    tty_busy[i] = TTY_FREE;
  }
}

/*######### Misc Functions #########*/
int get_next_pid() {
  return ++PID;
}

/* ###################### Switching Functions ################################ */

/*
 * Context switch from fork
 */
SavedContext* switchfunc_fork(SavedContext *ctxp, void *p1, void *p2 ){
  struct pcb *parent = p1;
  struct pcb *child = p2;
  struct pte *parent_table = (parent->page_table_p);
  struct pte *child_table = (child->page_table_p);
  clone_page_table(parent_table, &child_table);
  return ctxp;
}

/*
 * Initiate context for idle.
 * Return old context since we're not really context switching
 * Copy the kernel stack but map to new phsyical frame
 * because we can't share the kernel stack
 */
SavedContext* switchfunc_idle(SavedContext *ctxp, void *p1, void *p2){
  dprintf("in switchfunc_idle...", 1);
  int i;
  struct pcb *p = (struct pcb *) p1;
  struct pte *page_table = (p->page_table_p);
  // save context
  *(p->context) = *ctxp;
  page_table = p->page_table_p;
  // copy kernel stack
  int kernel_limit = get_page_index(KERNEL_STACK_LIMIT);
  int kernel_base = get_page_index(KERNEL_STACK_BASE);
  // check we have enough free memory to allocate new kernel stack
  debug_frames(0);
  // kernel stack is 4 pages long
  if (4 > len_free_frames()){
    unix_error("don't have enough physical space to create new process");
  }
  for(i=kernel_base; i<=kernel_limit; i++) {
    (page_table + i)->valid = (page_table0_p + i)->valid;
    (page_table + i)->pfn = get_free_frame();
    (page_table + i)->kprot = (page_table0_p + i)->kprot;
    (page_table + i)->uprot = (page_table0_p + i)->uprot;
  }
  // update registers & flush
  /*WriteRegister( REG_PTR0, (RCS421RegVal) page_table);*/
  /*WriteRegister( REG_TLB_FLUSH, TLB_FLUSH_0);*/
  return ctxp;
}

/*
 * Initial context switch into init program
 * Return old context since no new context exists yet
 * Take the current kernel stack and associate frame numbers of current
 * kernel stack with current process.
 * Basically, the kernel is already operating in the context of
 * the init program, and now the init program just needs to assume it.
 */
SavedContext* switchfunc_init(SavedContext *ctxp, void *p1, void *p2){
  dprintf("in switchfunc_idle...", 1);
  int i;
  struct pcb *p = (struct pcb *) p1;
  struct pte *page_table;
  // save context
  *(p->context) = *ctxp;
  page_table = p->page_table_p; // should be a completely reset page table
  // extract kernel stack
  int kernel_limit = get_page_index(KERNEL_STACK_LIMIT);
  int kernel_base = get_page_index(KERNEL_STACK_BASE);
  for(i=kernel_base; i<=kernel_limit; i++) {
    (page_table + i)->valid = (page_table0_p + i)->valid;
    (page_table + i)->pfn = (page_table0_p + i)->pfn;
    (page_table + i)->kprot = (page_table0_p + i)->kprot;
    (page_table + i)->uprot = (page_table0_p + i)->uprot;
  }
  // Update register & flush
  page_table0_p = page_table;
  WriteRegister( REG_PTR0, (RCS421RegVal) page_table0_p);
  WriteRegister( REG_TLB_FLUSH, TLB_FLUSH_0);
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
 * Save the context of process 1 into it's pcb
 */
SavedContext *switchfunc_normal(SavedContext *ctxp, void *pcb1, void *pcb2) {
  dprintf("in switchfunc_normal...", 1);
  int i;
  struct pte *page_table;
  struct pcb *p1 = (struct pcb *)pcb1;
  struct pcb *p2 = (struct pcb *)pcb2;
  // save context of current process
  // we want to save a COPY, not just the pointer!
  *(p1->context) = *ctxp;
  page_table = p1->page_table_p; // should be a completely reset page table
  // copy kernel stack
  int kernel_limit = get_page_index(KERNEL_STACK_LIMIT);
  int kernel_base = get_page_index(KERNEL_STACK_BASE);
  // check we have enough free memory to allocate new kernel stack
  debug_frames(0);
  // kernel stack is 4 pages long
  if (4 > len_free_frames()){
    unix_error("don't have enough physical space to create new process");
  }
  for(i=kernel_base; i<=kernel_limit; i++) {
    (page_table + i)->valid = (page_table0_p + i)->valid;
    (page_table + i)->pfn = get_free_frame();
    (page_table + i)->kprot = (page_table0_p + i)->kprot;
    (page_table + i)->uprot = (page_table0_p + i)->uprot;
  }
  // update registers & flush
  page_table0_p = p2->page_table_p;
  WriteRegister( REG_PTR0, (RCS421RegVal) page_table0_p);
  WriteRegister( REG_TLB_FLUSH, TLB_FLUSH_0);
  //TODO: not sure if this is right
  // Set regs & psr
  /*p2->frame->pc = p2->pc_next;*/
  /*p2->frame->sp = p2->sp_next;*/
  /*p2->frame->psr = p2->psr_next;*/
  /*if (p2->frame != NULL) {*/
    /*p2->frame = NULL;*/
    /*p2->pc_next = NULL;*/
    /*p2->sp_next = NULL;*/
    /*p2->psr_next = -1;*/
  /*}*/
  // return context of pcb2
  // this context needs to have been created by a call to context switch
  // return address of context of second process
  return p2->context;
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
  dprintf("in initswitchfunction...", 1);

  // Get page table
  struct pcb *p = (struct pcb *) p1;
  struct pte *page_table = (p->page_table_p);
  p->context = ctxp;
  //page_table = init_page_table0(page_table);

  //clone_page_table(page_table0_p, &page_table);
  page_table = init_page_table0(page_table);

  // debug
  debug_pcb(p);
  debug_page_table(page_table, 1);
  // Update current page table
  dprintf("writing ptr0 reg...", 0);
  page_table0_p = page_table;
  WriteRegister( REG_PTR0, (RCS421RegVal) page_table0_p);

  // flush out old entries
  if (VM_ENABLED) {
    dprintf("flusing region0...", 0);
    WriteRegister( REG_TLB_FLUSH, TLB_FLUSH_0);
  }
  return ctxp;
}


/*
 * Switch the current pcb to the next process in the ready queue.
 * If no process is ready, switch to the idle process
 */
void get_next_ready_process(struct pte *page_table) {
  dprintf("in get next ready process...", 1);
  elem *e;
  e = dequeue(p_ready);
  // if no ready process, switch to idle
  if (NULL == e) {
    dprintf("switching idle...", 1);
    // check if current process is free'd (exit)
    if (NULL == pcb_current) {
      dprintf("current proccess no longer exists", 1);
      // context switch into idle. Orignal program no longer exists
      if (0 > ContextSwitch(switchfunc_normal, pcb_idle->context, NULL , pcb_idle)) {
        unix_error("failed context switch");
      }
      // context switched to idle and old program has terminated.
      // should now be safe to free to old page tables
      dprintf("freeing old page table...", 1);
      free(page_table); //TODO
      dprintf("finished freeing old page table...", 0);
    } else {
      dprintf("current process is delayed...", 0);
      debug_pcb(pcb_idle);
      // current process is running but delayed
      // it is already in the delay queue and just needs to be contex switched out
      if (0 > ContextSwitch(switchfunc_normal, pcb_current->context, pcb_current, pcb_idle)) {
        unix_error("failed context switch");
      }
    }
    // finished context switching, now set the current pcb to idle to complete the switch
    dprintf("activating pcb of idle...", 1);
    pcb_current = pcb_idle;
  } else {
    // there is a process waiting to run so run that instead of idle
    dprintf("found a ready process...", 1);
    // switch to the new process, and save state of old process
    if (0 > ContextSwitch(switchfunc_normal, pcb_current->context, pcb_current, e->value)) {
      unix_error("failed context switch");
    }
    // finish context switching, set current pcb to new process
    dprintf("activating pcb of ready ...", 1);
    pcb_current = (struct pcb *)e->value;
  }
  dprintf("exit get_next_ready_process", 1);
}
