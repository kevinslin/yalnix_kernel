#include <stdio.h>
#include <stdlib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int
main(int argc, char **argv)
{
	printf("hello, i'm in init\n");
	Delay(3);
	printf("hello, i'm done with init\n");
	//Exit(0);
	return 0;
}
