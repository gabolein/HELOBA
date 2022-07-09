#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "src/protocol/message.h"
#include "src/protocol/message_parser.h"
#include <stdbool.h>
#include <stdint.h>

bool transport_initialize(void);
bool transport_change_frequency(uint16_t frequency);
bool transport_send_message(message_t *msg);
bool transport_receive_message(message_t *msg);
bool transport_get_id(uint8_t *buffer);

#endif
