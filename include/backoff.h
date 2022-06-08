#ifndef BACKOFF_H
#define BACKOFF_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "boilerplate.h"

typedef struct _backoff_struct{
  uint8_t attempts;
  uint32_t backoff_ms;
  struct timespec start_backoff;
} backoff_struct;

bool send_ready(msg_priority_queue_t*);
void set_new_backoff(backoff_struct*);
bool check_backoff_timeout(backoff_struct*);

#endif
