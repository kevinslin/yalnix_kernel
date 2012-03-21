#include <stdio.h>
#include <stdlib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

#define MAX_ARGC	32

int
main (int argc, char **argv) {
  printf("hello from init\n");
  Delay(3);
  printf("goodbye from init\n");
  return 0;
}
