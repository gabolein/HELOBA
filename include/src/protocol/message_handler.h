#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include "lib/datastructures/generic/generic_vector.h"
#include "src/protocol/message.h"

typedef bool (*filter_f)(message_t *msg);
MAKE_SPECIFIC_VECTOR_HEADER(message_t, message)

bool handle_message(message_t *msg);
void message_assign_collector(unsigned timeout_ms, unsigned max_messages,
                              filter_f filter, message_vector_t *collector);

#endif