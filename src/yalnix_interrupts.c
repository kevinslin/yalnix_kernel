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
  printf("got a kernel interrupt\n");
	switch(frame->code) {
		case YALNIX_FORK:
			break;
		case YALNIX_EXIT:
			printf("sys call exit\n");
			//TODO: get actual status
			Exit(0);
			break;
		default:
			printf("got unknown system call!\n");
	}

  //debug_stack_frame(frame);
}

void interrupt_clock(ExceptionStackFrame *frame){
}
void interrupt_illegal(ExceptionStackFrame *frame){
}
void interrupt_memory(ExceptionStackFrame *frame){
}
void interrupt_math(ExceptionStackFrame *frame){
}
void interrupt_tty_receive(ExceptionStackFrame *frame){
}
void interrupt_tty_transmit(ExceptionStackFrame *frame){
}
void interrupt_disk(ExceptionStackFrame *frame){
}

/*
 * Exit current process
 */
void Exit(int status) {
	ExceptionStackFrame *frame = pcb_current->frame;
	//free_pcb(pcb_current);
	printf("current process exiting..\n");
	start_idle(frame);
}

