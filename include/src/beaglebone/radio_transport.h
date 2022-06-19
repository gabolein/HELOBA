#ifndef RADIO_TRANSPORT_H
#define RADIO_TRANSPORT_H

#include <stdbool.h>
#include <stdint.h>

bool radio_change_frequency(uint16_t frequency);
bool radio_receive_packet(uint8_t *buffer, unsigned *length);
bool radio_send_packet(uint8_t *buffer, unsigned length);
bool radio_get_id(uint8_t *buffer);

#endif