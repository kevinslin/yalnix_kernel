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

	dprintf("in interrupt_kernel...", 0);
	switch(frame->code) {
		case YALNIX_FORK:
			printf("syscall fork...\n");
			break;
		case YALNIX_EXEC:
			dprintf("syscall exec...", 0);
			break;
		case YALNIX_EXIT:
			printf("sys call exit\n");
			//TODO: get actual status
			Exit(0);
			break;
		case YALNIX_WAIT:
			printf("syscall wait...\n");
			break;
		case YALNIX_GETPID:
			printf("syscall getting pid...\n");
      frame->regs[0] = pcb_current->pid;
			break;
		case YALNIX_BRK:
			printf("syscall getting brk...\n");
			break;
		case YALNIX_DELAY:
			printf("syscall delay... \n");
			is_valid = Delay(frame->regs[1]);
      frame->regs[0] = is_valid;
			break;
		default:
			printf("got unknown system call!\n");
	}
  //debug_stack_frame(frame);
}

void interrupt_clock(ExceptionStackFrame *frame){
	/*printf("got clock interrupt...\n");*/
  /*
   * Go through all the processors in waiting queue and decrement time delay.
   * If one reaches 0, move to ready queue.
   */
}
void interrupt_illegal(ExceptionStackFrame *frame){
  kernel_error("illegal instruction", frame);
}
void interrupt_memory(ExceptionStackFrame *frame){
  dprintf("in interrupt_memory", 0);
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

/*
 * Exit current process
 */
void Exit(int status) {
	dprintf("in exit...", 0);
	ExceptionStackFrame *frame = pcb_current->frame;
	free_pcb(pcb_current); //FIXME: implement
	start_idle(frame);
	exit(1); //FIXME: right?
}

int Delay(int clock_ticks) {
	printf("[info]:clock_ticks: %i\n", clock_ticks);
  if (0 == clock_ticks) return 0;
  if (0 > clock_ticks) return ERROR;
  pcb_current->time_delay = clock_ticks;
  //TODO: start a new process
	return 1;
}
