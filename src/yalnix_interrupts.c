#include "yalnix_interrupts.h"

/*
#define	YALNIX_FORK		1
#define	YALNIX_EXEC		2
#define	YALNIX_EXIT		3
#define	YALNIX_WAIT		4
#define YALNIX_GETPID           5
#define	YALNIX_BRK		6
#define	YALNIX_DELAY		7
#define	YALNIX_TTY_READ		21
#define	YALNIX_TTY_WRITE	22
*/
extern void start_idle(ExceptionStackFrame *frame);

void
interrupt_kernel(ExceptionStackFrame *frame) {
  int is_valid;
  // update current frame
  frame_current = frame;
	switch(frame->code) {
		case YALNIX_FORK:
			printf("syscall fork...\n");
			break;
		case YALNIX_EXEC:
			dprintf("syscall exec...", 0);
			break;
		case YALNIX_EXIT:
      dprintf("syscall exit...", 0);
			Exit(frame->regs[1]);
			break;
		case YALNIX_WAIT:
      dprintf("syscall wait...", 0);
      /*result = Wait((int *)frame->regs[1]);*/
      /*dprintf("got back from wait...", 0);*/
      /*frame->regs[0] = result;*/
			break;
		case YALNIX_GETPID:
			printf("syscall getting pid...\n");
      frame->regs[0] = pcb_current->pid;
			break;
		case YALNIX_BRK:
			printf("syscall getting brk...\n");
			break;
		case YALNIX_DELAY:
      dprintf("syscall delay...", 0);
			is_valid = Delay(frame->regs[1]);
      frame->regs[0] = is_valid;
			break;
    case YALNIX_TTY_READ:
      dprintf("sycall tty read...", 0);
      break;
    case YALNIX_TTY_WRITE:
      dprintf("syscall tty_write...", 0);
      break;
		default:
			printf("got unknown system call!\n");
	}
  //debug_stack_frame(frame);
}

/*
 * Clock interrupt decrements everything in the delay queue
 */
void interrupt_clock(ExceptionStackFrame *frame){
  printf("got clock interrupt...\n");
  /*
   * Go through all the processors in waiting queue and decrement time delay.
   * If one reaches 0, move to ready queue.
   */
  int i;
  elem *elem_c;
  struct pcb *pcb_p;
  struct pcb *pcb_tmp;
  elem_c = p_delay->head;

  // decrement items in delay queue
  // move into ready if time_delay has run down
  for (i=0; i < p_delay->len; i++) {
    pcb_p = (struct pcb *)elem_c->value;
    // decrement delay queue processors
    pcb_p->time_delay = pcb_p->time_delay - 1;
    // process finished delaying, put back in ready
    if (pcb_p->time_delay <= 0) {
      printf("move element from delay into ready...\n");
      pcb_tmp = (struct pcb *)pop(p_delay, pcb_p);
      enqueue(p_ready, (void *) pcb_tmp);
    }
    elem_c = elem_c->next;
  } // end for loop
  // context switch if process has been running for longer then 2 clock ticks
  pcb_current->time_current = pcb_current->time_current + 1;
  // don't context switch if idle is the only process
  if ((2 <= pcb_current->time_current) && (0 <= p_ready->len)) {
    // we need to reset the current time and put it into ready
    pcb_current->time_current = 0;
    // NEVER stick idle in queue
    if (pcb_current != pcb_idle) enqueue(p_ready, (void *)pcb_current);
    get_next_ready_process(pcb_current->page_table_p);
  }
}

void interrupt_illegal(ExceptionStackFrame *frame){
  kernel_error("illegal instruction", frame);
}
void interrupt_memory(ExceptionStackFrame *frame){
  dprintf("in interrupt_memory", 0);
	int f;
	int i;
  struct pte *page_table;

	unsigned int addr = ((long)frame->addr);
	int addr_page = get_page_index(addr);
  page_table = pcb_current->page_table_p;

	if(addr_page > pcb_current->brk_index && addr_page<pcb_current->stack_limit_index){
		if(addr_page - pcb_current->brk_index <= 1){
			printf("Stack overflow, address 0x%061x \n", addr);
			Exit(ERROR);
		}
		if(pcb_current->stack_limit_index - addr_page > len_free_frames()){
			printf("Not enough physical memory. address 0x%061x \n", addr);
			Exit(ERROR);
		}
		int k = 0;
		for(i = 0; i<pcb_current->stack_limit_index - addr_page; i++){
		  f = get_free_frame();
          (page_table + pcb_current->stack_limit_index - i-1)->valid = PTE_VALID;
          (page_table + pcb_current->stack_limit_index - i-1)->pfn = f;
          (page_table + pcb_current->stack_limit_index - i-1)->uprot = (PROT_READ | PROT_WRITE);
          (page_table + pcb_current->stack_limit_index - i-1)->kprot = (PROT_READ | PROT_WRITE);
		  //Not sure about the actual index
		  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
		  k++;
		}
		pcb_current->stack_limit_index = pcb_current->stack_limit_index - k - 1;
	}
	else{
		printf("Process %d: NOOBS ACCESSING INVALID MEMORY AT 0x%061x WITH CODE %d\n", pcb_current->pid, addr, frame->code);
		Exit(ERROR);
	}
}

void interrupt_math(ExceptionStackFrame *frame){
  kernel_error("illegal math operation", frame);
}

void interrupt_tty_receive(ExceptionStackFrame *frame){
  dprintf("got tty receive...", 0);
  stream *stream_read = (stream *)malloc(sizeof(stream));
  void *buf = malloc(TERMINAL_MAX_LINE);
  struct pcb *pcb_p;

  if (NULL == stream_read) Exit(ERROR);
  if (NULL == buf) Exit(ERROR);

  stream_read->buf = buf;
  stream_read->length = TtyReceive(frame->code, stream_read->buf, TERMINAL_MAX_LINE);
  enqueue(tty_read[frame->code], (void *)stream_read);

  if (0 < tty_read_wait[frame->code]->len) {
    pcb_p = (struct pcb *)dequeue(tty_read_wait[frame->code]);
    enqueue(p_ready, (void *)pcb_p);
  }
}

void interrupt_tty_transmit(ExceptionStackFrame *frame){
  dprintf("in interrupt_tty_transmit...", 0);
  stream *stream_write;
  stream *stream_tmp;
  struct pcb *pcb_p;
  int id;

  id = frame->code;
  stream_write = (stream *)dequeue(tty_write[id]);
  free(stream_write->buf);
  free(stream_write);

  assert(0 < tty_write_wait[id]->len);
  pcb_p = (struct pcb *)dequeue(tty_write_wait[id]);
  tty_busy[id] = TTY_FREE;
  enqueue(p_ready, pcb_p);

  // are any other lines that need to be written?
  if (0 < tty_write[id]->len) {
    stream_tmp = ((stream *)tty_write[id]->head);
    tty_busy[id] = TTY_BUSY;
    TtyTransmit(id, stream_tmp->buf, stream_tmp->length);
  }

}

void interrupt_disk(ExceptionStackFrame *frame){
  kernel_error("illegal disk access", frame);
}

/*
 * Dump data on yalnix system error
 */
void kernel_error(char *msg, ExceptionStackFrame *frame) {
  printf("%s \n", msg);
  printf("pid: %i\n", pcb_current->pid);
  printf("error code: %i\n", frame->code);
  Exit(ERROR);
}
