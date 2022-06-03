#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>

#define PACKET_STATUS_LENGTH 2
#define RXFIFO_ADDRESS 0x3F
#define TXFIFO_ADDRESS 0x3F
#define MAX_PACKET_LENGTH 0xFF
#define FIRST_BYTE_WAIT_TIME 2
#define NEXT_BYTE_WAIT_TIME 1

uint8_t receive_packet(uint8_t *);
void send_packet(uint8_t *);

#endif
