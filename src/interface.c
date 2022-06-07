#include "interface.h"
#include "boilerplate.h"
#include <stdio.h>
#include <stdlib.h>

void insert_input(char* data, size_t* len, msg_priority_queue_t* q){
    scanf("%255s%n", data, (uint32_t*)len); 
    msg* new_msg = create_msg(*len, data);
    msg_priority_queue_push(q, new_msg);
}

void* collect_user_input(void* args){
  msg_priority_queue_t* q = (msg_priority_queue_t*)args;
  char msg_data[256];
  msg_data[255] = '\n';
  size_t len = 0;

  while(1){
    insert_input(msg_data, &len, q);
  }
}
