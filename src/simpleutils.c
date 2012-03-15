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

