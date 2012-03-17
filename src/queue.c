#include "queue.h"

/*
 * Create queue that is queue max size
 */
queue *create_queue() {
  queue *q = (queue *)malloc(sizeof(queue));
  q->len = 0;
  q->size = QUEUE_MAX_SIZE;
  return q;
}

/*
 * Add an element into the queue.
 * Return -1 if full.
 */
int enqueue(queue *q, void *value) {
  // No more space
  if (q->len >= q->size) return -1;

  // Create queue element
  elem *elem_new= (elem *)malloc(sizeof(elem));
  elem_new->value = value;
  elem_new->next = NULL;

  // insert into empty queue
  if (0 == q->len) {
    q->head = elem_new;
    q->tail = elem_new;
  } else {
    q->tail->next = elem_new;
  }
  // book keeping
  q->len = q->len + 1;
  q->tail = elem_new;

  //sanity...
  assert(q->len <= q->size);
  return 1;
}

/*
 * Remove an element from queue
 * Return NULL if empty
 */
elem *dequeue(queue *q) {
  // Empty queue
  if (q->len == 0) return NULL;

  // elem wrapper
  elem *e = (elem *)q->head;

  // book keeping
  q->head = q->head->next;
  q->len = q->len - 1;
  return e;
}

/*
 * Debug queue value
 */
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
}
