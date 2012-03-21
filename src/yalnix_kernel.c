#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "yalnix_mem.h"
#include <string.h>

extern void start_idle(ExceptionStackFrame *frame);
extern LoadProgram(char *name, char **args, ExceptionStackFrame *frame, struct pcb **pcb_p);

int Exec(char *filename, char **argvec){
	return LoadProgram(filename, argvec, pcb_current->frame, &pcb_current);
}

/*
 * Exit current process
 */
void Exit(int status) {
	struct pte *page_table;
	dprintf("in exit...", 0);
	//ExceptionStackFrame *frame = pcb_current->frame;
	page_table = terminate_pcb(pcb_current); //FIXME: implement
	get_next_ready_process(page_table);
	exit(status); //FIXME: right?
}

int Delay(int clock_ticks) {
  dprintf("in delay...", 1);
	// set delay on current proccess
  if (0 == clock_ticks) return 0;
  if (0 > clock_ticks) return ERROR;
  pcb_current->time_delay = clock_ticks;
  debug_pcb(pcb_current); //DEBUG
	// Put current process in delay
	if (0 > enqueue(p_delay, (void *) pcb_current)) return ERROR;
	get_next_ready_process(pcb_current->page_table_p);
  dprintf("exit delay...", 1);
	return 0;
}

/*
 * Called when user mallocs
 */
int Brk(void *addr){
	dprintf("in brk...", 0);
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
	dprintf("in fork...", 1);
	struct pcb *parent = pcb_current;
  // create pcb for child
	struct pcb *child = Create_pcb(parent);
	child->brk_index = parent->brk_index;
	child->stack_limit_index = parent->stack_limit_index;
  child->name = "child process";

  SavedContext *ctx;
  ctx = &child->context;
  // create page tables for child
  dprintf("creating pages for child...", 2);
	struct pte* page_table = create_page_table();
	child->page_table_p = page_table;

  dprintf("enqueing child...", 2);
	enqueue(p_ready, (void *)child);

  dprintf("about to context switch...", 2);
	if(ContextSwitch(switchfunc_fork, &child->context, parent, child) == -1) {
	}
	if(pcb_current == parent) {
		return child->pid;
	} else if(pcb_current == child) {
		return child->pid;
	} else {
		return ERROR;
	}
	return ERROR;
}

/*
 * Wait for children to finish
 */
int Wait(int *status) {
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
	// get status of children
	p = (struct pcb *)dequeue(pcb_current->children_wait);
	pid = p->pid;
	status = &p->status;
  free_pcb(p);
	return pid;
}

/*
 * Reads length chars from stream into buf of terminal id
 */
int TtyRead(int id, void *buf, int length) {
	stream *s;
	int length_diff;
	// no lines ready to read
	while (0 == tty_read[id].len) {
		enqueue(&tty_read_wait[id], pcb_current);
		get_next_ready_process(pcb_current->page_table_p);
	}
	// something's ready to read
	s = (stream *)dequeue(&tty_read[id]);

	length_diff = length - s->length;
	// read everything
	if (0 <= length_diff) {
		length = s->length;
	}
	memcpy(buf, s->buf, length);
	if (0 <= length_diff) {
		memmove(s->buf, s->buf + length, length_diff);
		s->length = length_diff;
		//TODO: put at front of queue
	} else {
		// we're done reading
		free(s->buf);
		free(s);
	}
	return length;
}

/*
 * Writes length chars from buf into terminal id
 */
int TtyWrite(int id, void *buf, int length) {
  dprintf("got ttywrite...", 1);
	void *buf_tmp;
	stream *s;
	// error checking
	if ( (0 > id) || (NUM_TERMINALS <= id)) return ERROR;
	if (0 > length) return ERROR;
	//TODO: check buffer

	buf_tmp = malloc(length);
	if (NULL == buf_tmp) return ERROR;
	s = malloc(sizeof(stream));
	if (NULL == s) return ERROR;

	s->length = length;
	s->buf = buf_tmp;
	memcpy(s->buf, buf, length);

	// make sure terminal is not busy
	if (TTY_BUSY != tty_busy[id]) {
		tty_busy[id] = TTY_BUSY;
		TtyTransmit(id, s->buf, s->length);
	}
  dprintf("enqueing stream and pcb...", 2);
	enqueue(&tty_write[id], s);
	enqueue(&tty_write_wait[id], (void *)pcb_current);

  dprintf(BOND, 2);
  debug_tty_queues(id);
  debug_pcb(pcb_current);
  debug_pcb( tty_write_wait[id].head );
  dprintf(BOND, 2);

	get_next_ready_process(pcb_current->page_table_p);
	return s->length;
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

