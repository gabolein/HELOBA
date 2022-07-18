#include "src/protocol/message_util.h"
#include "lib/time_util.h"
#include "src/transport.h"

void collect_messages(unsigned timeout_ms, unsigned max_messages,
                      filter_f filter, message_vector_t *collector) {
  message_t msg;
  struct timespec start_time;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
  while (!hit_timeout(timeout_ms, &start_time)) {
    if (transport_receive_message(&msg) && filter(&msg)) {
      message_vector_append(collector, msg);
      if (message_vector_size(collector) == max_messages) {
        return;
      }
    }
  }
}