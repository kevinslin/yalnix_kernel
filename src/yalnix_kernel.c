#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "yalnix_mem.h"

extern void start_idle(ExceptionStackFrame *frame);
/*
 * Exit current process
 */
void Exit(int status) {
	struct pte *page_table;
	dprintf("in exit...", 0);
	//ExceptionStackFrame *frame = pcb_current->frame;
	page_table = terminate_pcb(pcb_current); //FIXME: implement
	get_next_ready_process(page_table);
	//exit(1); //FIXME: right?
}

int Delay(int clock_ticks) {
	// set delay on current proccess
	/*printf("[info]:clock_ticks: %i\n", clock_ticks);*/
  if (0 == clock_ticks) return 0;
  if (0 > clock_ticks) return ERROR;
  pcb_current->time_delay = clock_ticks;

	// Put current process in delay
	if (0 > enqueue(p_delay, (void *) pcb_current)) {
		unix_error("error enqueing process!");
	}
	get_next_ready_process(pcb_current->page_table_p);
	return 0;
}

/*
 * Called when user mallocs
 */
int Brk(void *addr){
	int user_heap_limit_index;
	int addr_index;
	int pages_needed;
	int i;
	int frame;
	struct pte *page_table = pcb_current->page_table_p;

	user_heap_limit_index = pcb_current->brk_index;
	addr_index = get_page_index(addr);
	pages_needed = addr_index - user_heap_limit_index;
	if (pages_needed > 0) {
		if (len_free_frames() >= pages_needed) {
			int k = 0;
			for (i = 0; i < pages_needed; i++) {
				frame = get_free_frame();
				(page_table + user_heap_limit_index + i)->valid = PTE_VALID;
				(page_table + user_heap_limit_index + i)->pfn = frame;
				(page_table + user_heap_limit_index + i)->uprot = PROT_NONE;
				(page_table + user_heap_limit_index + i)->kprot = (PROT_READ | PROT_WRITE);
				k++;
			}
			pcb_current->brk_index = user_heap_limit_index + k;
			WriteRegister( REG_TLB_FLUSH, TLB_FLUSH_0);
			dprintf("finished allocating memory", 0);
			return 0;
		} else {
			int k = 0;
			// space is written, again
			for (i = 0; i < pages_needed; i++) {
				(page_table + user_heap_limit_index - i)->valid = PTE_INVALID;
				set_frame((page_table + user_heap_limit_index - i)->pfn, FRAME_FREE);
				(page_table + user_heap_limit_index- i)->pfn = 0;
				(page_table + user_heap_limit_index - i)->uprot = PROT_NONE;
				(page_table + user_heap_limit_index - i)->kprot = PROT_NONE;
				k++;
			}
			pcb_current->brk_index = user_heap_limit_index - k;
			WriteRegister( REG_TLB_FLUSH, TLB_FLUSH_0);
		}
	}
	return 0;
}


int Fork(){
	dprintf("in fork...", 0);
	struct pcb *parent = pcb_current;
	struct pcb *child = Create_pcb(parent);
	child->brk_index = parent->brk_index;
	child->stack_limit_index = parent->stack_limit_index;
	SavedContext *ctx = child->context;

	enqueue(p_ready, child);

	if(ContextSwitch(switchfunc_fork, ctx, parent, child) == -1) {
		return ERROR;
	}
	if(pcb_current == parent) {
		return child->pid;
	} else if(pcb_current == child) {
		return 0;
	} else {
		return ERROR;
	}
	return ERROR;
}

/*
 * Wait for children to finish
 */
int Wait(int *status) {
	elem *e;
	struct pcb * p;
	unsigned int pid;

	while (0 >= pcb_current->children_wait->len) {
		// check if process has children in first place
		if (0 >= pcb_current->children_active->len){
			unix_error("current process has no children!");
		}
		// current process is waiting
		pcb_current->status = STATUS_WAIT;
		// context switch
		enqueue(p_waiting, (void *) pcb_current);
		get_next_ready_process(pcb_current->page_table_p);
	}
	e = dequeue(pcb_current->children_wait);
	// get status of children
	p = (struct pcb *) e;
	pid = p->pid;
	status = &p->status;
  free_pcb(p);
	return pid;
}

/*
extern int Fork(void);
extern int Exec(char *, char **);
extern void Exit(int) __attribute__ ((noreturn));
extern int GetPid(void);
extern int Brk(void *);
extern int Delay(int);
extern int TtyRead(int, void *, int);
extern int TtyWrite(int, void *, int);
extern int Register(unsigned int);
extern int Send(void *, int);
extern int Receive(void *);
extern int ReceiveSpecific(void *, int);
extern int Reply(void *, int);
extern int Forward(void *, int, int);
extern int CopyFrom(int, void *, void *, int);
extern int CopyTo(int, void *, void *, int);
extern int ReadSector(int, void *);
extern int WriteSector(int, void *);
*/

