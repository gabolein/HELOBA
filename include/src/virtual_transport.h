#ifndef VIRTUAL_TRANSPORT_H
#define VIRTUAL_TRANSPORT_H

#include <stdbool.h>
#include <stdint.h>

bool virtual_transport_initialize();
bool virtual_change_frequency(uint16_t frequency);
bool virtual_send_packet(uint8_t *buffer, unsigned length);
bool virtual_receive_packet(uint8_t *buffer, unsigned *length);
bool virtual_get_id(uint8_t *out);

#endif