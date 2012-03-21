#include "yalnix_mem.h"

#define PAGEMASK2 ((VMEM_1_SIZE / PAGESIZE) -1)
extern void start_idle();
extern struct pte *page_table1_p;
/* Globals */
// incremented every time a new process is started
int PID = 0;
struct pte pte_null = {0, 0, 0, 0, 0};
struct pte pte_mask = {-1, 0, 0, 0, 0};


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
  for (i=MEM_INVALID_PAGES; i<NUM_FRAMES; i++) {
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
 *
 * Maintain a one to one mapping between virtual and physical
 * address. "new" is index of VA of region1.
 */
struct pte *create_page_table() {
	if(page_table_free->len == 0){
  // find a free frame to put the page table in
  long old = DOWN_TO_PAGE(page_brk) / PAGESIZE;
  page_brk -= PAGESIZE;
  long new = DOWN_TO_PAGE(page_brk) / PAGESIZE;
  // edge case where rounding down references the same page
  if (old == new) {
    //return reset_page_table((struct pte *) page_brk);
	  return create_page_table_helper((void *)page_brk);
  }
  // frames_p + new == new
  if ((frames_p + new)->free == FRAME_NOT_FREE) {
    unix_error("can't create page table");
  } else {
    set_frame(new, FRAME_FREE); // maybe redundant
    int offset = new - (VMEM_1_BASE / PAGESIZE); // index into pte1
    (page_table1_p + offset)->valid = PTE_VALID;
    (page_table1_p + offset)->pfn = new;
    (page_table1_p + offset)->kprot = (PROT_READ | PROT_WRITE);
    (page_table1_p + offset)->uprot = PROT_NONE;
	//return reset_page_table((struct pte *) page_brk);
	WriteRegister(REG_TLB_FLUSH, page_brk);
    return create_page_table_helper((void *)page_brk);
  }
	}
	else
		return create_page_table_helper(dequeue(page_table_free));
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

struct pte *init_page_table1(struct pte *page_table) {
  int i;
  int limit = get_page_index(KERNEL_STACK_LIMIT);
  int base = get_page_index(KERNEL_STACK_BASE);
  for(i=base; i<=limit; i++) {
	int free_frame = get_free_frame();
    (page_table + i)->valid = PTE_VALID;
    (page_table + i)->pfn = free_frame;
    (page_table + i)->kprot = (PROT_READ | PROT_WRITE);
    (page_table + i)->uprot = PROT_NONE;
    set_frame(free_frame, FRAME_NOT_FREE);
  }
  return page_table;
}

/*


 * Clone pt2 into pt1
 * Mallocs room for new page table TODO: possible memory leak
 */
struct pte *clone_page_table(struct pte *src, struct pte *dest) {
  int i;
  for (i=0; i<PAGE_TABLE_LEN; i++) {
    (dest + i)->valid = (src +i)->valid;
    (dest + i)->uprot = (src +i)->uprot;
    (dest + i)->kprot = (src +i)->kprot;
	if((src + i)->valid == PTE_VALID){
		int free_frame = get_free_frame();
		(dest+i)->pfn = free_frame;
		(page_table1_p +  PAGEMASK2)->valid = PTE_VALID;
		(page_table1_p + PAGEMASK2)->kprot = (PROT_READ | PROT_WRITE);
		(page_table1_p + PAGEMASK2)->uprot = PROT_NONE;
		(page_table1_p + PAGEMASK2)->pfn = free_frame;
		WriteRegister( REG_TLB_FLUSH, VMEM_1_BASE + PAGEMASK2 * PAGESIZE);

		/*
		 * Copy memory of old process into the vpn of
		 * the copy buffer. Since this corresponds to
		 * vpn of idle kernel vpn, this change should be
		 * propagated into the idle kernel stack.
		 */
		void *dst = (void *)(VMEM_1_BASE + PAGEMASK2 * PAGESIZE);
		//void *src1 = (void *)((((long)(src + i) *PAGESIZE)&PAGEMASK3) >> 12);
		void *src1 = (void *)((long)(i * PAGESIZE));
		memcpy(dst, src1 , PAGESIZE);

		/*
		 * Copy buffer served its purpose, invalidate
		 * so that it can be used again in the future
		 */
		(page_table1_p + PAGEMASK2)->valid = PTE_INVALID;
		(page_table1_p + PAGEMASK2)->kprot = PROT_NONE;
		(page_table1_p + PAGEMASK2)->uprot = PROT_NONE;
		(page_table1_p + PAGEMASK2)->pfn = PFN_INVALID;
		WriteRegister( REG_TLB_FLUSH, VMEM_1_BASE + PAGEMASK2 * PAGESIZE);
	  }
	else
		(dest +i)->pfn = 0;

  }
  return dest;
}

/*
 * Sets everything to invalid
 */
struct pte *reset_page_table(struct pte *page_table) {
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
  free(pcb_p->children_wait);
  free(pcb_p->children_active); //TODO: free elemetns in queue
 // free(pcb_p->context);
  free(pcb_p);
  return 1;
}

/*
 * Detroy pcb and update book keeping information
 */
struct pte *terminate_pcb(struct pcb *pcb_p) {
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
  // zombie status need to be collected by parent
  pcb_p->status = STATUS_ZOMBIE;
  if (NULL != pcb_p->parent) {
    enqueue(pcb_p->parent->children_wait, pcb_p);
    if (STATUS_WAIT == pcb_p->parent->status) {
      //TODO: remove parent from waiting queue
      // add removed parent to the ready queue
    }
  } else {
    free_pcb(pcb_p);
  }
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
  dprintf("Starting fork switch", 9);
  struct pcb *parent = p1;
  struct pcb *child = p2;
  struct pte *parent_table = parent->page_table_p;
  struct pte *child_table = child->page_table_p;
  clone_page_table(parent_table, child_table);
  return ctxp;
}

/*
 * Initiate context for idle.
 * Return old context since we're not really context switching
 * Copy the kernel stack but map to new phsyical frame
 * because we can't share the kernel stack
 */


/*
 * Initial context switch into init program
 * Return old context since no new context exists yet
 * Take the current kernel stack and associate frame numbers of current
 * kernel stack with current process.
 * Basically, the kernel is already operating in the context of
 * the init program, and now the init program just needs to assume it.
 */
SavedContext* switchfunc_init(SavedContext *ctxp, void *pcb1, void *pcb2){
	struct pcb *p1 = (struct pcb *)pcb1;
	struct pte *page_table = p1->page_table_p;
    clone_page_table(page_table0_p, page_table);
	//page_table = init_page_table1(page_table);
	if(page_table == NULL)
		exit(-1);
	page_table0_p = page_table;
	WriteRegister(REG_PTR0, (RCS421RegVal)page_table0_p);
	if(VM_ENABLED){
		WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
	}

	return ctxp;
}

/*
 * Returns context and nothign else
 */
SavedContext* switchfunc_nop(SavedContext *ctxp, void *p1, void *p2 ) {
  return ctxp;
}

/*
 * Switch from process 1 to process 2
 * Save the context of process 1 into it's pcb
 */
SavedContext *switchfunc_normal(SavedContext *ctxp, void *pcb1, void *pcb2) {

  struct pcb *p2 = (struct pcb *)pcb2;

  page_table0_p = p2->page_table_p;



  WriteRegister( REG_PTR0, (RCS421RegVal) page_table0_p);
  WriteRegister( REG_TLB_FLUSH, TLB_FLUSH_0);
  if(p2->frame != NULL){
	  p2->frame->pc = p2->pc_next;
	  p2->frame->sp = p2->sp_next;
	  p2->frame->psr = p2->psr_next;
	  p2->frame = NULL;
	  p2->pc_next = NULL;
	  p2->sp_next = NULL;
	  p2->psr_next = -1;


  }
  /*return ctxp;*/
  SavedContext* c = &(p2->context);
  return c;
}



/*
 * Switch the current pcb to the next process in the ready queue.
 * If no process is ready, switch to the idle process
 */
void get_next_ready_process(struct pte *page_table) {
  elem *e;
  e = dequeue(p_ready);
  // if no ready process, switch to idle
 if (NULL == e) {
	if (NULL == pcb_current) {
		SavedContext context;
		dprintf("About to switch",9);
		pcb_current = idle_pcb;
		if (0 > ContextSwitch(switchfunc_normal, &context, NULL , idle_pcb)) {
			unix_error("failed context switch");
		}
		dprintf("Switched",9);
		page_table = reset_page_table(page_table);
		enqueue(page_table_free, page_table);
    } else {
	dprintf("Switching to idle",9);
	  SavedContext *ctx = &(pcb_current->context);
	  struct pcb *pw = pcb_current;
	  pcb_current =idle_pcb;
      if (0 > ContextSwitch(switchfunc_normal, ctx, pw, idle_pcb)) {
        unix_error("failed context switch");
      }
	  dprintf("Got back from switch", 9);
	}
  } else {
    dprintf("Switching normal",9);
	struct pcb *p = e->value;
	if(p->page_table_p != NULL)
		dprintf("I have a page table 2", 9);
	SavedContext *ctx = &(pcb_current->context);
	struct pcb *pw = pcb_current;
	pcb_current = (struct pcb *)e->value;
    if (0 > ContextSwitch(switchfunc_normal, ctx, pw, e->value)) {
      unix_error("failed context switch");
    }

 }
  dprintf("exit get_next_ready_process", 1);
}

