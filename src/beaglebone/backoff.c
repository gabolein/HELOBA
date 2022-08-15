#define _POSIX_C_SOURCE 199309L
#include "src/beaglebone/backoff.h"
#include "lib/datastructures/generic/generic_priority_queue.h"
#include "lib/random.h"
#include "src/beaglebone/rssi.h"
#include "src/beaglebone/radio_transport.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#define TWO_PWR_OF(n) (1 << n)

static backoff_struct node_backoff = {0, 0, {0}};

void set_new_backoff() {
  node_backoff.attempts++;
  node_backoff.backoff_ms =
      random_number_between(0, TWO_PWR_OF(node_backoff.attempts) - 1);
  clock_gettime(CLOCK_MONOTONIC_RAW, &node_backoff.start_backoff);
}

bool check_backoff_timeout() {
  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC_RAW, &current);

  size_t delta_ms =
      (current.tv_sec - node_backoff.start_backoff.tv_sec) * 1000 +
      (current.tv_nsec - node_backoff.start_backoff.tv_nsec) / 1000000;

  return delta_ms >= node_backoff.backoff_ms;
}

bool collision_detection() {
  if (radio_channel_active(5)) {
    set_new_backoff();
    return false;
  }
  return true;
}

uint8_t get_backoff_attempts() { return node_backoff.attempts; }

void reset_backoff_attempts() { node_backoff.attempts = 0; }
