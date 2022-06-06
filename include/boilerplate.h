#ifndef BOILERPLATE_H
#define BOILERPLATE_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

typedef struct _queue_elem{
  void* val;
  struct _queue_elem* next;
} queue_elem;

typedef struct _queue{
  queue_elem* first;
  queue_elem* last;
} queue;

typedef struct _msg{
  uint8_t len;
  uint8_t data[255];
} msg;

queue* create_queue(void);
void enqueue(queue*, queue_elem*);
queue_elem* dequeue(queue*);
queue_elem* peek_queue(queue*);
msg* create_msg(size_t, char*);

#endif
