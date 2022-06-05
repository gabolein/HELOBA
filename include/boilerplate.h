#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include "backoff.h"

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

typedef struct _node_msg_struct{
  msg message;
  backoff_struct backoff;
} node_msg_struct;

queue* create_queue(void);
void enqueue(queue*, queue_elem*);
queue_elem* dequeue(queue*);
queue_elem* peek_queue(queue*);
node_msg_struct* create_msg(size_t, char*);

queue* msg_queue;
