#ifndef TRANSFER_H
#define TRANSFER_H

#include "src/protocol/message.h"

typedef bool (*handler_f)(message_t *msg);
void register_automatic_transfer_handlers();

bool perform_split(split_direction);
bool perform_unregistration(frequency_t to);
bool perform_registration(frequency_t to);
bool handle_do_split(message_t *);
bool handle_will_transfer(message_t *);
bool handle_do_transfer(message_t *);

#endif
