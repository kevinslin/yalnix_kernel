#include <stdio.h>
#include <stdlib.h>
#include "simpleutils.c"

int
main() {
  printf("hello world!\n");
  //s_print("s_print message");
  dprintf("you can't see me", 0);
  dprintf("d print message", 2);
  return 1;
}
