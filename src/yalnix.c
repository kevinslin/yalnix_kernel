#include <stdio.h>
#include <stdlib.h>

#include <comp421/yalnix.h>
#include <comp421/hardware.h>


void KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args) {
  printf("hello world!");
}

int SetKernelBrk(void *size) {
  return 0;
}

