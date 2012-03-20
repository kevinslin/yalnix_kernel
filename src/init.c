#include <stdio.h>
#include <stdlib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int
main(int argc, char **argv)
{
  printf("##########################################################################\n");
	printf("hello, i'm in init\n");
  printf("##########################################################################\n");
	Delay(3);
  printf("##########################################################################\n");
	printf("hello, i'm done with init\n");
  printf("##########################################################################\n");
	/*Exit(0);*/
	return 0;
}
