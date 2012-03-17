#include <stdio.h>
#include <stdlib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int a = 5, c = 15;

int
main()
{

    int b = 10, d = 20;
    int *foo;

    Delay(20);
    /* Pause(); */

    TracePrintf(2, "init!\n");

    printf("hello world\n");
    printf("this %d is %d a %d test %d\n", a, b, c, d);
    printf("the pid is: %i\n", GetPid());
    foo = (int *)malloc(sizeof(int));

    for (;;) {
    }

    Exit(3);
}
