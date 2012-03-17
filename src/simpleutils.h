/*
 * Useful debug functions
 */

#ifndef _simpleutils_h
#define _simpleutils_h

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <comp421/hardware.h>

#define DEBUG_LEVEL 0

#define DIVIDER "=================\n"


void dprintf(char *msg, int level);
void unix_error(char *msg);


#endif /* end _simpleutils_h */


