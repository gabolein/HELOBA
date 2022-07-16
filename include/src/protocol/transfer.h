#ifndef TRANSFER_H
#define TRANSFER_H

#include "src/protocol/message.h"

bool perform_split(void);
bool perform_unregistration(frequency_t to);
bool perform_registration();
bool handle_do_split(message_t *);
bool handle_will_transfer(message_t *);
bool handle_do_transfer(message_t *);

#endif
