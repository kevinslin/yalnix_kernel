#include "bootstrap.h"

void KernelStart(ExceptionStackFrame *, unsigned int, void *, char **){
  printf("hello world!");
}


void main(int argc, char **argv){
  KernelStart();
  exit(0);
}
