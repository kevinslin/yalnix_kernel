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

#define DEBUG_LEVEL 1

#define DIVIDER "=================\n"
#define BOND "##############################################################\n"


void dprintf(char *msg, int level);
void unix_error(char *msg);


#endif /* end _simpleutils_h */


