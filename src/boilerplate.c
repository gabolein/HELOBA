#include "boilerplate.h"
#include <stdlib.h>
#include <string.h>

int cmp(msg* message1, msg* message2){
  if (message1->prio > message2->prio)
    return 1;
  if (message1->prio < message2->prio)
    return -1;
  return 0;
}

MAKE_SPECIFIC_PRIORITY_QUEUE_SOURCE(msg*, msg, cmp);

msg* create_msg(size_t len, char* data){
  msg* new_msg = malloc(sizeof(msg));
  new_msg->len = len;
  strncpy((char*)new_msg->data, data, 255);

  return new_msg;
}
