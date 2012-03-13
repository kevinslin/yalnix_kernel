#define FRAME_NOT_FREE -1
#define FRAME_FREE 1

#define PTE_VALID 1
#define PTE_INVALID 0
#define PFN_INVALID  -1

#define TABLE1_OFFSET 512

typedef struct {
  int free;
} page_frames;

static int **interrupt_vector_table[TRAP_VECTOR_SIZE];
static page_frames *frames_p;

int NUM_FRAMES;

int len_free_frames();
