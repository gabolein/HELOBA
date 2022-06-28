#define _POSIX_C_SOURCE 199309L
#include "lib/time_util.h"
#include <stdio.h>

void sleep_ms(uint32_t to_sleep) {
  struct timespec to_sleep_ts = {0, 0};
  to_sleep_ts.tv_sec = to_sleep / 1e3;
  to_sleep_ts.tv_nsec = to_sleep * 1e6 - to_sleep_ts.tv_sec * 1e9;
  nanosleep(&to_sleep_ts, NULL);
}

bool hit_timeout(size_t ms, struct timespec *start) {
  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC_RAW, &current);

  size_t delta_ms = (current.tv_sec - start->tv_sec) * 1000 +
                    (current.tv_nsec - start->tv_nsec) / 1000000;
  return delta_ms >= ms;
}
