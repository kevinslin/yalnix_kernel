#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "yalnix_mem.h"

extern void start_idle(ExceptionStackFrame *frame);
/*
 * Exit current process
 */
void Exit(int status) {
	dprintf("in exit...", 0);
	ExceptionStackFrame *frame = pcb_current->frame;
	free_pcb(pcb_current); //FIXME: implement
	start_idle(frame);
	//exit(1); //FIXME: right?
}

int Delay(int clock_ticks) {
	printf("[info]:clock_ticks: %i\n", clock_ticks);
  if (0 == clock_ticks) return 0;
  if (0 > clock_ticks) return ERROR;
  pcb_current->time_delay = clock_ticks;
  //TODO: start a new process
  //ContextSwitch(switchfunction, pcb_current->context, pcb_current);
	return 1;
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
	struct pte *page_table = pcb_current->page_table;

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
	struct pcb *parent = pcb_current;
	struct pcb *child = Create_pcb(parent);
	child->brk_index = parent->brk_index;
	child->stack_limit_index = parent->stack_limit_index;
	SavedContext *ctx = child->context;

	//push child on ready queue when queues are implemented
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
 * Switch from process 1 to process 2
 */
/*SavedContext *switchfunction(SavedContext *ctxp, void *p1, void *p2) {*/
  /*SavedContext *c;*/
  /*struct pcb *p1 = (struct pcb *)p1;*/
  /*struct pcb *p2 = (struct pcb *)p2;*/
/*}*/
