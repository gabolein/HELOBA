#include "interface.h"
#include "boilerplate.h"
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <assert.h>

#define MAX_MESSAGE_LENGTH 255
void insert_input(char* data, size_t* len, msg_priority_queue_t* q){
  char user_input[MAX_MESSAGE_LENGTH+1];
  fgets(user_input, MAX_MESSAGE_LENGTH+1, stdin);
  sscanf(user_input, "%255s%n", data, (uint32_t*)len); 
  __fpurge(stdin);
  msg* new_msg = create_msg(*len, data);
  msg_priority_queue_push(q, new_msg);
}

void* collect_user_input(void* args){
  msg_priority_queue_t* q = (msg_priority_queue_t*)args;
  char msg_data[MAX_MESSAGE_LENGTH+1];
  msg_data[MAX_MESSAGE_LENGTH] = '\n';
  size_t len = 0;

  for(;;){
    insert_input(msg_data, &len, q);
  }
}
