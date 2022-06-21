#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <stdbool.h>
#include <stdint.h>

bool change_frequency(uint16_t frequency);
bool send_packet(uint8_t *buffer, unsigned length);
bool receive_packet(uint8_t *buffer, unsigned *length);
bool listen(uint8_t *buffer, unsigned *length, unsigned listen_ms);

#endif
