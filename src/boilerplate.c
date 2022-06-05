#include "boilerplate.h"
#include <stdlib.h>
#include "string.h"

queue* create_queue(){
  return calloc(1,sizeof(queue));
}

void enqueue(queue* q, queue_elem* new_elem){
  if(!q || !new_elem){
    return;
  }

  if(!q->first){
    q->first = new_elem;
  } else {
    q->last->next = new_elem;
  }
  q->last = new_elem;
  new_elem->next = NULL;
}

queue_elem* dequeue(queue* q){
  if(!q || !q->first)
    return NULL;

  if(q->first == q->last){
    q->last = NULL;
  }
  queue_elem* elem = q->first;
  q->first = q->first->next;
  return elem;
}

queue_elem* peek_queue(queue* q){
  return q->first;
}

node_msg_struct* create_msg(size_t len, char* data){
  node_msg_struct* new_msg_struct = malloc(sizeof(node_msg_struct));
  new_msg_struct->message.len = len;
  strncpy((char*)new_msg_struct->message.data, data, 255);
  new_msg_struct->backoff.attempts = 0;
  new_msg_struct->backoff.backoff_ms = 0;
  struct timespec time = {0, 0};
  new_msg_struct->backoff.start_backoff = time;

  return new_msg_struct;
}
