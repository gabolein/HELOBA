#ifndef BACKOFF_H
#define BACKOFF_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef struct _backoff_struct {
  uint8_t attempts;
  uint32_t backoff_ms;
  struct timespec start_backoff;
} backoff_struct;

void set_new_backoff();
bool check_backoff_timeout();
bool collision_detection();
uint8_t get_backoff_attempts();
void reset_backoff_attempts();

#endif
