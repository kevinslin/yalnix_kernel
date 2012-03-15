#ifndef _simpleutils_h
#define _simpleutils_h

#include <stdio.h>
#include <comp421/hardware.h>

#define DEBUG_LEVEL 0

#define DIVIDER "=================\n"
//#define s_print(msg) printf("%s\n", msg)
//#define dprintf(msg, level) ( (level) >= (DEBUG_LEVEL) ? s_printf(msg) : (printf("")) )


void dprintf(char *msg, int level);


#endif /* end _simpleutils_h */


