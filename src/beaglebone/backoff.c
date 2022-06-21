#define _POSIX_C_SOURCE 199309L
#include <stddef.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include "src/beaglebone/backoff.h"
#include "src/beaglebone/rssi.h"
#include "lib/datastructures/generic/generic_priority_queue.h"

#define TWO_PWR_OF(n)(1 << n)

static backoff_struct node_backoff = {0, 0, {0}};

size_t random_number_between(size_t min, size_t max) {
  assert(min <= max);
  srand(time(NULL));
  return min + (rand() % (int)(max - min + 1));
}

void set_new_backoff(){
  node_backoff.attempts++;
  node_backoff.backoff_ms = random_number_between(0, TWO_PWR_OF(node_backoff.attempts)-1);
  clock_gettime(CLOCK_MONOTONIC_RAW, &node_backoff.start_backoff);
}

bool check_backoff_timeout(){
  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC_RAW, &current);

  size_t delta_ms = (current.tv_sec - node_backoff.start_backoff.tv_sec) * 1000 +
    (current.tv_nsec - node_backoff.start_backoff.tv_nsec) / 1000000;

  return delta_ms >= node_backoff.backoff_ms;
}

bool collision_detection(){
  if(detect_RSSI()){
    set_new_backoff();
    return false;
  }
  return true;
}

uint8_t get_backoff_attempts(){
  return node_backoff.attempts;
}

void reset_backoff_attempts(){
  node_backoff.attempts = 0;
}
