#define _GNU_SOURCE  
#include "packet.h"
#include "frequency.h"
#include "time_util.h"
#ifndef VIRTUAL
#include "registers.h"
#include "rssi.h"
#include <SPIv1.h>
#endif
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
// For virtual mode
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <signal.h>
#include <poll.h>

bool eq(uint32_t a, uint32_t b) { return a == b; }

unsigned h1(uint32_t key) {
  key = ((key >> 16) ^ key) * 0x45d9f3b;
  key = ((key >> 16) ^ key) * 0x45d9f3b;
  key = (key >> 16) ^ key;
  return key;
}

unsigned h2(uint32_t key) {
  key += ~(key << 15);
  key ^= (key >> 10);
  key += (key << 3);
  key ^= (key >> 6);
  key += ~(key << 11);
  key ^= (key >> 16);
  return key;
}

//MAKE_SPECIFIC_VECTOR_SOURCE(freq_socket_hash_entry_t, freq_socket_hashentry)
MAKE_SPECIFIC_HASHMAP_SOURCE(uint32_t, int, freqsocket, h1, h2, eq)

#ifndef VIRTUAL
uint8_t rx_fifo_length() {
  int recv_bytes;
  cc1200_reg_read(NUM_RXBYTES, &recv_bytes);
  return (uint8_t)recv_bytes;
}

uint8_t rx_fifo_pop() {
  int read_byte;
  cc1200_reg_read(RXFIFO_ADDRESS, &read_byte);
  return (uint8_t)read_byte;
}

void tx_fifo_push(uint8_t value) { cc1200_reg_write(TXFIFO_ADDRESS, value); }

bool fifo_wait(size_t ms){
  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
  while (!hit_timeout(ms, &start)){
    if (rx_fifo_length() != 0) {
      return true;
    }
  }
  return false;
}
#endif

uint8_t receive_packet_virtual(uint8_t *buffer, uint32_t current_frequency) {
  int node_socket = freqsocket_hashmap_get(global_socket_map, current_frequency);
  buffer[0] = 0;
  struct pollfd fd;
  int ret;

  fd.fd = node_socket;
  fd.events = POLLIN;
  ret = poll(&fd, 1, 100);
  switch(ret){
    case -1:
      perror("poll");
      return 0;
    case 0:
      return 0;
    default:
      recv(node_socket, buffer, MAX_PACKET_LENGTH*sizeof(*buffer), 0);
      break;
  }

  return buffer[0];
}

#ifndef VIRTUAL
uint8_t receive_packet(uint8_t *buffer) {
  start_receiver_blocking();

  //while (rx_fifo_length() == 0) {
  //};
  uint8_t packet_length;

  if(!fifo_wait(FIRST_BYTE_WAIT_TIME)){
    packet_length = 0;
  } else {

    packet_length = rx_fifo_pop();
    printf("Packet length is %u\n", packet_length);

    for (size_t i = 0; (uint16_t)i < (packet_length + PACKET_STATUS_LENGTH);
         i++) {
      /*while (rx_fifo_length() == 0) {*/
      /*};*/
      if(!fifo_wait(NEXT_BYTE_WAIT_TIME)){
        packet_length = 0;
        break;
      } else {
        buffer[i] = rx_fifo_pop();
      }
    }

    if(packet_length){
      uint8_t *status = buffer + packet_length;
      printf("RSSI: %i, CRC: %s, Link Quality: %u\n", status[0],
             status[1] & (1 << 7) ? "OK" : ":(", status[1] & ~(1 << 7));
    }
  }

  return packet_length;
}
#endif

void send_packet_virtual(uint8_t *buffer, uint32_t current_frequency){
  int node_socket = freqsocket_hashmap_get(global_socket_map, current_frequency);

  if(sendto(node_socket, buffer, buffer[0], 0, (struct sockaddr*)&global_broadcast_addr, sizeof(global_broadcast_addr)) < 0){
    perror("Send to error");
  }
}
  

#ifndef VIRTUAL
// NOTE: sollte man die Länge explizit dazu geben oder sollte man einfach
// annehmen, dass der erste Wert im buffer die Länge ist?
void send_packet(uint8_t *buffer) {  
  for (size_t i = 0; i < buffer[0]; i++) {
    tx_fifo_push(buffer[i]);
  }

  cc1200_cmd(STX);
}
#endif

bool init_virtual_interfaces(uint32_t node_id){
  uint32_t available_frequencies[] = {840, 860, 880, 900, 920};


  if(!(global_socket_map = freqsocket_hashmap_create(eq, h1, h2))){
    return false;
  }

  struct ifreq ifr;
  for (size_t i = 0; i < 5; i++){
    printf("Creating socket connecting to interface eth%u.%u\n", available_frequencies[i], node_id);
    int node_socket;
    if((node_socket = socket(AF_PACKET, SOCK_DGRAM, htons(0x88b5))) == -1){
          perror("socket");
          return false;
    }
    char if_name[10];
    snprintf(if_name, 10, "eth%u.%u", available_frequencies[i], node_id);
    strcpy(ifr.ifr_name, if_name);

    if(ioctl(node_socket, SIOCGIFINDEX, &ifr) == -1){
        perror("ioctl");
        return false;
    }

    freqsocket_hashmap_insert(global_socket_map, available_frequencies[i], node_socket);
  }

  global_broadcast_addr.sll_family = AF_PACKET;
  global_broadcast_addr.sll_ifindex = ifr.ifr_ifindex;
  global_broadcast_addr.sll_halen = ETHER_ADDR_LEN;
  global_broadcast_addr.sll_protocol = htons(0x88b5);
   /*11111111111... eth broadcast*/
  uint8_t dest[] = {0xff,0xff,0xff,0xff,0xff,0xff};
  memcpy(global_broadcast_addr.sll_addr, dest, ETHER_ADDR_LEN);

  return true;
}
