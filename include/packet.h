#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include <netpacket/packet.h>
#include "lib/datastructures/generic/generic_hashmap.h"

#define PACKET_STATUS_LENGTH 2
#define RXFIFO_ADDRESS 0x3F
#define TXFIFO_ADDRESS 0x3F
#define MAX_PACKET_LENGTH 0xFF
#define FIRST_BYTE_WAIT_TIME 2
#define NEXT_BYTE_WAIT_TIME 1

MAKE_SPECIFIC_HASHMAP_HEADER(uint32_t, int, freqsocket)

freqsocket_hashmap_t* global_socket_map;
struct sockaddr_ll global_broadcast_addr;

uint8_t receive_packet(uint8_t *);
uint8_t receive_packet_virtual(uint8_t *, uint32_t);
void send_packet_virtual(uint8_t *, uint32_t);
bool init_virtual_interfaces(uint32_t);

#endif
