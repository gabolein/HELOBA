#ifndef VIRTUAL_TRANSPORT_H
#define VIRTUAL_TRANSPORT_H

#include <stdbool.h>
#include <stdint.h>

bool virtual_interfaces_initialize(uint8_t virtid);
bool virtual_change_frequency(uint16_t frequency, uint8_t virtid);
bool virtual_send_packet(uint8_t *buffer, unsigned length);
bool virtual_receive_packet(uint8_t *buffer, unsigned *length);

#endif