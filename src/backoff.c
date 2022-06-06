#define _POSIX_C_SOURCE 199309L
#include <stddef.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include "backoff.h"
#include "rssi.h"
#include "packet.h"

static backoff_struct node_backoff = {0, 0, 0};

size_t random_number_between(size_t min, size_t max) {
  assert(min <= max);
  srand(time(NULL));
  return min + (rand() % (int)(max - min + 1));
}

void set_new_backoff(backoff_struct* backoff){
  backoff->attempts++;
  backoff->backoff_ms = random_number_between(0, (1 << backoff->attempts)-1);
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

bool send_ready(queue* q) {
  msg* current_msg = peek_queue(q)->val;

  if(!current_msg || !check_backoff_timeout(&node_backoff))
    return false;

  if(!collision_detection(&node_backoff))
    return false;

  send_packet((uint8_t*)&current_msg);
  node_backoff.attempts = 0;

  if(!collision_detection(&node_backoff))
    return false;

  dequeue(q);
  return true;
}
