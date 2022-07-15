#ifndef MESSAGE_UTIL_H
#define MESSAGE_UTIL_H

#include "src/protocol/message.h"

typedef bool (*filter_f)(message_t *msg);

void collect_messages(unsigned timeout_ms, unsigned max_messages,
                      filter_f filter, message_vector_t *collector);

#endif