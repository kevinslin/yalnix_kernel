#ifndef _yalnix_mem_h
#define _yalnix_mem_h

#define PTE_VALID 1
#define PTE_INVALID 0
#define PFN_INVALID  -1

#define TABLE1_OFFSET 512

typedef struct {
  int free;
} page_frames;


static void *interrupt_vector_table[TRAP_VECTOR_SIZE];
static page_frames *frames_p;

int NUM_FRAMES;

int initialize_frames(int num_frames);
int set_frame(int index, int status);
int len_free_frames();
void debug_frames();

#endif	/* yalnix_mem.h */
