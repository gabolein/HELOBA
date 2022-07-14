#define _POSIX_C_SOURCE 199309L
#include "lib/time_util.h"
#include <stdio.h>

struct timespec ms_to_timespec(uint32_t ms) {
  struct timespec ret = {0, 0};
  ret.tv_sec = ms / 1e3;
  ret.tv_nsec = ms * 1e6 - ret.tv_sec * 1e9;
  return ret;
}

void sleep_ms(uint32_t to_sleep) {
  struct timespec to_sleep_ts = ms_to_timespec(to_sleep);
  nanosleep(&to_sleep_ts, NULL);
}

bool hit_timeout(size_t ms, struct timespec *start) {
  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC_RAW, &current);

  size_t delta_ms = (current.tv_sec - start->tv_sec) * 1000 +
                    (current.tv_nsec - start->tv_nsec) / 1000000;
  return delta_ms >= ms;
}

struct timespec timestamp_add_ms(struct timespec stamp, uint32_t to_add){
  struct timespec to_add_ts = ms_to_timespec(to_add);
  stamp.tv_sec += to_add_ts.tv_sec;
  stamp.tv_nsec += to_add_ts.tv_nsec;

  return stamp;
}

int timestamp_cmp(struct timespec stamp_a, struct timespec stamp_b){
  if (stamp_a.tv_sec > stamp_b.tv_sec)
    return 1;

  if (stamp_a.tv_sec < stamp_b.tv_sec)
    return -1;

  if (stamp_a.tv_nsec > stamp_b.tv_nsec)
    return 1;

  if (stamp_a.tv_nsec < stamp_b.tv_nsec)
    return -1;

  return 0;
}

