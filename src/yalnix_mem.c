#include "yalnix_mem.h"

/* Globals */
int PID = 0;

/* Debug functions */
void
debug_page_tables(struct pte *table, int verbosity) {
  int i;
  if (verbosity) {
    for (i=0; i<NUM_PAGES; i++) {
      printf("i:%i, pfn: %u, valid: %u\n", i, (table + i)->pfn, (table + i)->valid);
    }
  } // end verbosity
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
  struct pte *page_table = (struct pte *)malloc(sizeof(struct pte) * NUM_PAGES);
  if (page_table == NULL) {
    return NULL;
  }
  if (0 > reset_page_table(page_table)) {
    return NULL;
  }
  return page_table;
}

/*
 * Clone pt2 into pt1
 */
struct pte *clone_page_table(struct pte *src) {
  int i;
  struct pte *dest = create_page_table();
  for (i=0; i<NUM_PAGES; i++) {
    (dest + i)->pfn = (src + i)->pfn;
    (dest + i)->valid = (src +i)->valid;
    (dest + i)->uprot = (src +i)->uprot;
    (dest + i)->kprot = (src +i)->kprot;
  }
  return dest;
}

struct pte *reset_page_table(struct pte *page_table) {
  int i;
  for (i=0; i<NUM_PAGES; i++) {
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
  struct pcb *pcb_p = (struct pcb *)malloc(sizeof(struct pcb));
  if (pcb_p == NULL) {
    return NULL;
  }
  pcb_p->pid = get_next_pid();
  pcb_p->time_current = 0;
  pcb_p->time_delay = 0;
  pcb_p->status = 0;
  pcb_p->parent = parent;
  pcb_p->frame = NULL;
  pcb_p->pc_next = NULL;
  pcb_p->sp_next = NULL;
  pcb_p->psr_next = -1;
  return pcb_p;
}

//TODO
int free_pcb(struct pcb *pcb_p) {
  free(pcb_p);
  return 1;
}

struct pcb *Create_pcb(struct pcb *parent) {
  struct pcb *a_pcb;
  a_pcb = create_pcb(parent);
  if (a_pcb == NULL) {
    printf("error creating pcb\n");
    exit(1);
  }
  return a_pcb;
}
