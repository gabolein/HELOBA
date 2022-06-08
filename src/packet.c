#include "packet.h"
#include "frequency.h"
#include "registers.h"
#include "rssi.h"
#include "time_util.h"
#include <SPIv1.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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


uint8_t receive_packet_virtual(uint8_t *buffer) {
  // TODO set socket
  buffer[0] = 0;
  read(node_socket, buffer, MAX_PACKET_LENGTH);
  return buffer[0];
}

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

void send_packet_virtual(uint8_t *buffer){

  int node_socket;
  if((node_socket = socket(AF_PACKET, SOCK_DGRAM, htons(0x88b5))) == -1){
        perror("socket");
  }
  char if_name[10];
  snprintf(if_name, 10, "eth%u.%u", current_frequency, node_id);
  struct ifreq ifr;
  strcpy(ifr.ifr_name, if_name);
  if(ioctl(node_socket, SIOCGIFINDEX, &ifr) == -1){
      perror("ioctl");
  }

  struct sockaddr_ll broadcast_addr;
  broadcast_addr.sll_family = AF_PACKET;
  broadcast_addr.sll_ifindex = ifr.ifr_ifindex;
  broadcast_addr.sll_halen = ETHER_ADDR_LEN;
  broadcast_addr.sll_protocol = htons(0x88b5);
   /*11111111111... eth broadcast*/
  uint8_t dest[] = {0xff,0xff,0xff,0xff,0xff,0xff};
  memcpy(g_addr.sll_addr, dest, ETHER_ADDR_LEN);
  
  if(sendto(node_socket, buffer, buffer[0], 0, (struct sockaddr*)&node_addr, sizeof(g_addr)) < 0){
    perror("Send to error");
  }
}
  

// NOTE: sollte man die Länge explizit dazu geben oder sollte man einfach
// annehmen, dass der erste Wert im buffer die Länge ist?
void send_packet(uint8_t *buffer) {  
  for (size_t i = 0; i < buffer[0]; i++) {
    tx_fifo_push(buffer[i]);
  }

  cc1200_cmd(STX);
}
