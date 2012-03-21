/*
 * Yalnix interrupt functions
 *
 */

#ifndef _yalnix_interrupts_h
#define _yalnix_interrupts_h

#include <stdlib.h>
#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>

#include "yalnix_mem.h"
#include "simpleutils.h"


void interrupt_kernel(ExceptionStackFrame *frame);
void interrupt_clock(ExceptionStackFrame *frame);
void interrupt_illegal(ExceptionStackFrame *frame);
void interrupt_memory(ExceptionStackFrame *frame);
void interrupt_math(ExceptionStackFrame *frame);
void interrupt_tty_receive(ExceptionStackFrame *frame);
void interrupt_tty_transmit(ExceptionStackFrame *frame);
void interrupt_disk(ExceptionStackFrame *frame);

void kernel_error(char *msg, ExceptionStackFrame *frame);

struct pcb *get_waiting_process();

#endif /* end of _yalnix_interrupts_h */
