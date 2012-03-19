#ifndef _queue_h
#define _queue_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define QUEUE_MAX_SIZE 5

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

queue *create_queue();
int enqueue(queue *q, void *value);
elem *dequeue(queue *q);
void *pop(queue *q, void *value);
void debug_queue(queue *q);

#endif
