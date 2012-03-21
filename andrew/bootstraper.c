#include "bootstrap.h"

void KernelStart(exception_stack_frame sf){
  printf("hello world!");
}


void main(int argc, char **argv){
  KernelStart();
  exit(0);
}
