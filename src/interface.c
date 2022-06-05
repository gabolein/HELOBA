#include "interface.h"
#include "boilerplate.h"
#include <stdio.h>
#include <stdlib.h>

void* collect_user_input(void* args){
  if(args){
    puts("Hi mom");
  }
  char msg_data[256];
  msg_data[255] = '\n';
  size_t len = 0;

  while(1){
    scanf("%255s%n", msg_data, (uint32_t*)&len); 
    node_msg_struct* new_msg_struct = create_msg(len, msg_data);
    queue_elem* elem = malloc(sizeof(queue_elem));
    elem->val = (void*)new_msg_struct;
    enqueue(msg_queue, elem);
  }
}
