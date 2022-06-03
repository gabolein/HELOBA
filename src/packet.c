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

// NOTE: sollte man die Länge explizit dazu geben oder sollte man einfach
// annehmen, dass der erste Wert im buffer die Länge ist?
void send_packet(uint8_t *buffer) {  
  for (size_t i = 0; i < buffer[0]; i++) {
    tx_fifo_push(buffer[i]);
  }

  cc1200_cmd(STX);
}
