#ifndef BACKOFF_H
#define BACKOFF_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef struct _backoff_struct{
  uint8_t attempts;
  uint8_t backoff_ms;
  struct timespec start_backoff;
} backoff_struct;

bool send_ready(void);

#endif
