#include <comp421/hardware.h>
#include "yalnix_mem.h"

/* TODO: dirty hack */
bool VM_ENABLED;
/*
 *  Increment brk pointer
 *  Returns on success
 */
int SetKernelBrk(void *addr) {
	printf("in set kernel brk\n");
  printf("address value: %p\n", addr);

  if (VM_ENABLED) {
    //TODO
  } else {
    // vm is disabled, just move brk pointer up
    KERNEL_HEAP_LIMIT = addr;
  }
	return 0;
}
