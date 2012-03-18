#include "yalnix_interrupts.h"

/*
#define	YALNIX_FORK		1
#define	YALNIX_EXEC		2
#define	YALNIX_EXIT		3
#define	YALNIX_WAIT		4
#define YALNIX_GETPID           5
#define	YALNIX_BRK		6
#define	YALNIX_DELAY		7
*/
extern void start_idle(ExceptionStackFrame *frame);

void
interrupt_kernel(ExceptionStackFrame *frame) {
  int is_valid;
	switch(frame->code) {
		case YALNIX_FORK:
			printf("syscall fork...\n");
			break;
		case YALNIX_EXEC:
			dprintf("syscall exec...", 0);
			break;
		case YALNIX_EXIT:
			printf("sys call exit\n");
      //TODO(TEST):
      start_idle(frame);
			//TODO: get actual status
			Exit(0);
			break;
		case YALNIX_WAIT:
      dprintf("syscall wait...", 0);
      Wait((int *)frame->regs[1]);
			break;
		case YALNIX_GETPID:
			printf("syscall getting pid...\n");
      frame->regs[0] = pcb_current->pid;
			break;
		case YALNIX_BRK:
			printf("syscall getting brk...\n");
			break;
		case YALNIX_DELAY:
      /*dprintf("syscall delay", 0);*/
			is_valid = Delay(frame->regs[1]);
      frame->regs[0] = is_valid;
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
	/*printf("got clock interrupt...\n");*/
  /*
   * Go through all the processors in waiting queue and decrement time delay.
   * If one reaches 0, move to ready queue.
   */
  int i;
  elem *elem_c;
  struct pcb *pcb_p;
  elem_c = p_delay->head;

  // decrement items in delay queue
  // move into ready if time_delay has run down
  for (i=0; i < p_delay->len; i++) {
    pcb_p = (struct pcb *)elem_c->value;
    // decrement delay queue processors
    pcb_p->time_delay = pcb_p->time_delay - 1;
    if (pcb_p->time_delay <= 0) {
      //TODO: remove it from delay
      // add removed process to ready
    }
    // context switch if process has been running for longer then 2 clock ticks
    pcb_p->time_current = pcb_p->time_current + 1;
    // don't context switch if idle is the only process
    if (2 <= pcb_p->time_current) && (0 >= p_ready->len){
      // we need to reset the current time and put it into ready
      pcb_p->time_current = 0;
      enqueue(pcb_p, p_ready);
      get_next_ready_process();
    }
    elem_c = elem_c->next;
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
}
void interrupt_tty_transmit(ExceptionStackFrame *frame){
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
