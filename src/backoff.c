#define _POSIX_C_SOURCE 199309L
#include <stddef.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include "backoff.h"
#include "rssi.h"
#include "packet.h"
#include "lib/datastructures/generic/generic_priority_queue.h"

#define TWO_PWR_OF(n)(1 << n)

static backoff_struct node_backoff = {0, 0, 0};

size_t random_number_between(size_t min, size_t max) {
  assert(min <= max);
  srand(time(NULL));
  return min + (rand() % (int)(max - min + 1));
}

void set_new_backoff(backoff_struct* backoff){
  backoff->attempts++;
  backoff->backoff_ms = random_number_between(0, TWO_PWR_OF(backoff->attempts)-1);
  clock_gettime(CLOCK_MONOTONIC_RAW, &backoff->start_backoff);
}

bool check_backoff_timeout(backoff_struct* backoff){
  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC_RAW, &current);

  size_t delta_ms = (current.tv_sec - backoff->start_backoff.tv_sec) * 1000 +
    (current.tv_nsec - backoff->start_backoff.tv_nsec) / 1000000;

  return delta_ms >= backoff->backoff_ms;
}

bool collision_detection(backoff_struct* backoff){
  if(detect_RSSI()){
    set_new_backoff(backoff);
    return false;
  }
  return true;
}

bool send_ready(msg_priority_queue_t* q) {
  if(msg_priority_queue_size(q) == 0)
    return false;
  msg* current_msg = msg_priority_queue_peek(q);

  if((node_backoff.attempts > 0) && !check_backoff_timeout(&node_backoff))
    return false;

  if(!collision_detection(&node_backoff))
    return false;

  send_packet((uint8_t*)&current_msg->len);

  if(!collision_detection(&node_backoff))
    return false;

  msg_priority_queue_pop(q);
  node_backoff.attempts = 0;
  return true;
}
