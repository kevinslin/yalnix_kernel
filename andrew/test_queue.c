#include "queue.h"
#include "stdlib.h"
#include "stdio.h"

struct pcb{
  unsigned int pid;
  unsigned int time_current;
  unsigned int time_delay; // time to wait before process is restarted
  int status; // delayed, sleeping...
  void *brk; //heap limit DEPRECIATE?
  int brk_index; //heap limit
  void *stack_limit; // DEPRECIATE?
  int stack_limit_index;
  struct pte *page_table_p;
  struct pcb *parent;
  void *pc_next;
  void *sp_next;
  unsigned long psr_next;
  char *name; //for debugging purposes
};


struct pcb pcb_create_test() {
  struct pcb *pcb_p;
  // Initiate pcb
  pcb_p = (struct pcb *)malloc(sizeof(struct pcb));
  if (pcb_p == NULL) fprintf(stderr, "error creating pcb");
  pcb_p->time_current = 0;
  pcb_p->time_delay = 0;
  pcb_p->pc_next = NULL;
  pcb_p->sp_next = NULL;
  return *pcb_p;
}

int
main(int argc, char *argv[]) {
  queue *q;
  elem *e;
	struct pcb i, j, k;
	struct pcb *result;
  i = pcb_create_test(NULL);
	i.name = "i pcb";
  j = pcb_create_test(NULL);
	j.name = "j pcb";
  k = pcb_create_test(NULL);
	k.name = "k pcb";

  printf("hello\n");
  q = create_queue();
  // pop with value at head
  enqueue(q, &i);
  result = (struct pcb *)pop(q, &i);
  printf("result: %s\n", result->name);
  debug_queue(q);
  printf("=============\n");

	// pop with value at tail
	enqueue(q, &j);
	enqueue(q, &k);
	result = (struct pcb *)pop(q, &k);
	printf("result: %s\n", result->name);
	debug_queue(q);
	printf("=============\n");

	// pop value in the middle
	enqueue(q, &i);
	enqueue(q, &k);
	result = (struct pcb *)pop(q, &i);
	printf("result: %s\n", result->name);
	debug_queue(q);
	exit(1);


  /*e = dequeue(q);*/
  /*printf("dequed value: %i\n", *(int *)e->value);*/
  /*debug_queue(q);*/
  printf("done\n");
}
