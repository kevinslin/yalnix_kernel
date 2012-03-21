#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <uuid/uuid.h>

#define QUEUE_MAX_SIZE 100

#define DUMMY -1

typedef struct node{
  struct node *next;
  struct node *prev;
  void *value;
} elem;

typedef struct q{
  elem *head;
  elem *tail;
  unsigned int size;
  unsigned int len;
} queue;


queue *create_queue() {
  queue *q = (queue *)malloc(sizeof(queue));
  q->len = 0;
  q->size = QUEUE_MAX_SIZE;
  q->head = NULL;
  q->tail = NULL;
  return q;
}

int enqueue(queue *q, void *value) {
  // No more space
  if (q->len >= q->size) return -1;

  // Create queue element
  elem *elem_new= (elem *)malloc(sizeof(elem));
  elem_new->value = value;
  elem_new->next = NULL;

  //insert into empty queue
  if (0 == q->len) {
    q->head = elem_new;
    q->tail = elem_new;
    elem_new->prev = NULL;
    elem_new->next = NULL;
  } else {
    q->tail->next = elem_new;
    elem_new->prev = q->tail;
    elem_new->next = NULL;
  }
  q->len = q->len + 1;
  q->tail = elem_new;
  //sanity...
  assert(q->len <= q->size);
  return 1;
}

/*
 * Pop value from queue or return null
 */
void *pop(queue *q, void *value) {
  printf("in pop...\n");
  int i;
  elem *elem_c;

  elem_c = q->head;
  for (i=0; i< q->len; i++) {
    printf("in for loop\n");
    printf("value: %p\n", value);
    printf("elem: %p\n", elem_c->value);
    if (value == elem_c->value) {
      printf("found value...\n"); // one element queue
      if (1 == q->len) {
        q->head = NULL;
        q->tail = NULL;
      }
      // element is first thing in queue
      else if (q->head == elem_c) {
        q->head = elem_c->next;
        q->head->prev = NULL;
      }
      else if (q->tail == elem_c) {
        printf("found a tail...\n");
        q->tail = elem_c->prev;
        q->tail->next = NULL;
      }
      else {
        elem_c->prev->next = elem_c->next;
        elem_c->next->prev = elem_c->prev;
      }
      q->len = q->len - 1;
      //TODO: free elemc
      return elem_c->value;
    } //end if
    elem_c = elem_c->next;
  }
  printf("did not find value...\n");
  return NULL;
}

elem *dequeue(queue *q) {
  // Empty queue
  if (q->len == 0) return NULL;

  elem *e = (elem *)q->head;

  q->head = q->head->next;
  if (q->head != NULL) q->head->prev = NULL;
  q->len = q->len - 1;
  return e;
}

void debug_queue(queue *q) {
  int i;
  elem *elem_c;

  elem_c = q->head;
  for (i=0; i < q->len; i++) {
    printf("%i, %i\n", i, *(int *)elem_c->value);
    elem_c = elem_c->next;
  }
  printf("size: %u \n",  q->size);
  printf("len: %u \n", q->len);
  if (q->head == NULL)
    printf("head: NULL\n");
  else
    printf("head: %p\n", q->head->value);
  if (q->tail== NULL)
    printf("tail: NULL\n");
  else
    printf("tail: %p\n", q->tail->value);
}



int
main(int argc, char *argv[]) {
  queue *q;
  elem *e;
  int i, j, k, result;
  i = 5;
  j = 6;
  k = 7;

  printf("hello\n");
  q = create_queue();
  // pop with value at head
  enqueue(q, &i);
  result = *(int *)pop(q, &i);
  printf("result: %i\n", result);
  debug_queue(q);
  printf("=============\n");

  // pop with value at tail
  enqueue(q, &j);
  enqueue(q, &k);
  result = *(int *)pop(q, &k);
  printf("result: %i\n", result);
  debug_queue(q);
  printf("=============\n");

  // pop value in the middle
  enqueue(q, &i);
  enqueue(q, &k);
  result = *(int *)pop(q, &i);
  printf("result: %i\n", result);
  debug_queue(q);
  exit(1);


  e = dequeue(q);
  printf("dequed value: %i\n", *(int *)e->value);
  debug_queue(q);
  printf("done\n");
}
