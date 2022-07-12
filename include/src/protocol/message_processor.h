#ifndef MESSAGE_PROCESSOR_H
#define MESSAGE_PROCESSOR_H

#include "message.h"
#include "src/protocol/routing.h"

bool message_addressed_to(message_t *msg, routing_id_t id);
bool process_message(message_t *msg);

#endif
