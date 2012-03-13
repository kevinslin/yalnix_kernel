#include "yalnix_mem.h"


int
initialize_frames(int num_frames) {
	NUM_FRAMES = num_frames;
	int i;
	page_frames frames[NUM_FRAMES];
	frames_p = frames;
  for (i=0; i<NUM_FRAMES; i++) {
		(*(frames_p + i)).free = FRAME_FREE;
  }
	return 1;
}

/* Set frame free at given index */
int
set_frame(int index, int status) {
	(*(frames_p + index)).free = status;
	return 1;
}

/* Returns number of free frames in system */
int
len_free_frames() {
	int i;
	int count_free = 0;
	for (i=0; i<NUM_FRAMES; i++) {
		if ((*(frames_p + i)).free == FRAME_FREE) {
			count_free++;
		}
	}
	return count_free;
}


void
debug_frames() {
	int i;
	printf("base address: %p\n", frames_p);
	printf("lim address: %p\n", frames_p + NUM_FRAMES);
	printf("free frames: %i\n", len_free_frames());
	for(i=0; i<NUM_FRAMES; i++) {
		printf("address: %p ", frames_p + i);
		if ((frames_p + i)->free == FRAME_FREE) {
			printf("mem free\n");
		} else if ((frames_p + i)->free == FRAME_NOT_FREE) {
			printf("mem not free\n");
		} else {
			printf("invalid frame entry: %i\n",(frames_p + i)->free);
		}
	}
}



