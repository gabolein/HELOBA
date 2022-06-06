#include "interface.h"
#include "boilerplate.h"
#include <stdio.h>
#include <stdlib.h>

void insert_input(char* data, size_t* len, queue* q){
    scanf("%255s%n", data, (uint32_t*)len); 
    msg* new_msg = create_msg(*len, data);
    queue_elem* elem = malloc(sizeof(queue_elem));
    elem->val = (void*)new_msg;
    enqueue(q, elem);
}

void* collect_user_input(void* args){
  queue* q = (queue*)args;
  char msg_data[256];
  msg_data[255] = '\n';
  size_t len = 0;

  while(1){
    insert_input(msg_data, &len, q);
  }
}
