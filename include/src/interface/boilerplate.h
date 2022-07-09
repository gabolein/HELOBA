#ifndef BOILERPLATE_H
#define BOILERPLATE_H

#include "lib/datastructures/generic/generic_priority_queue.h"
#include <stddef.h>
#include <stdint.h>
#include <time.h>

typedef enum _msg_prio {
  //  MUTE,
  LEADER,
  CONTROL,
  DATA
} msg_prio;

typedef struct _msg {
  msg_prio prio;
  uint8_t len;
  uint8_t data[255];
} msg;

MAKE_SPECIFIC_PRIORITY_QUEUE_HEADER(msg *, msg)

msg *create_msg(size_t, char *);
int cmp_msg_prio(msg *, msg *);

#endif
