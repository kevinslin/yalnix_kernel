#include "simpleutils.h"


/*
 * Only print message if it is higher then debug level
 */
void
dprintf(char *msg, int level) {
  if (level >= DEBUG_LEVEL) {
    printf("%s\n", msg);
  }
}


void unix_error(char *msg) /* unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}
