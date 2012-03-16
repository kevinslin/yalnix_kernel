#include <stdio.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int
main() {
  for (;;){
    Pause();
    printf("idling...\n");
  }
  return 1;
}
