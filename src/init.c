
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

	int
main(int argc, char **argv)
{
	printf("i'm in fork\n");
	if (Fork() == 0) {
		printf("hello from child\n");
		TracePrintf(0, "CHILE\n");
	}
	else {
		printf("hello from parent");
		TracePrintf(10, "PARENT\n");
		Delay(8);
	}

	Exit(0);
}
