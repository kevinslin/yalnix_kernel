#include "simpleutils.h"


/*
 * Only print message if it is higher then debug level
 */
void
dprintf(char *msg, int level) {
  if (level >= DEBUG_LEVEL) {
    printf("[debug]:%s\n", msg);
  }
}


void unix_error(char *msg) /* unix-style error */
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}
