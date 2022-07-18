#ifndef MESSAGE_FORMATTER_H
#define MESSAGE_FORMATTER_H

#include "src/protocol/message.h"

void formatter_initialize();
void formatter_teardown();

char *format_action(message_action_t action);
char *format_type(message_type_t type);
char *format_routing_id(routing_id_t id);
char *format_message(message_t *msg);

#endif