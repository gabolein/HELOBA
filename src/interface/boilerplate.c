// TODO remove
#include "src/interface/boilerplate.h"
#include <stdlib.h>
#include <string.h>

#define MAX_PACKET_LENGTH 255

int cmp_msg_prio(msg *message1, msg *message2) {
  if (message1->prio > message2->prio)
    return 1;
  if (message1->prio < message2->prio)
    return -1;
  return 0;
}

MAKE_SPECIFIC_VECTOR_SOURCE(msg *, msg);
MAKE_SPECIFIC_PRIORITY_QUEUE_SOURCE(msg *, msg, cmp_msg_prio);

msg *create_msg(size_t len, char *data) {
  msg *new_msg = malloc(sizeof(msg));
  assert(len <= MAX_PACKET_LENGTH);
  new_msg->len = len;
  strncpy((char *)new_msg->data, data, MAX_PACKET_LENGTH);

  return new_msg;
}
