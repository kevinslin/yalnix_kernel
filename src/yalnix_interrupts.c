#include "yalnix_interrupts.h"


void
interrupt_kernel(ExceptionStackFrame *frame) {
  printf("got a kernel interrupt\n");
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
