#include "queue.h"

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
    printf("%i, %p\n", i, elem_c->value);
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
