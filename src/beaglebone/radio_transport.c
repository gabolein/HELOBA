#include "src/beaglebone/radio_transport.h"
#include "lib/time_util.h"
#include "src/beaglebone/frequency.h"
#include "src/beaglebone/registers.h"
#include "src/beaglebone/rssi.h"
#include "src/beaglebone/backoff.h"
#include <SPIv1.h>
#include <net/if.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#define PACKET_STATUS_LENGTH 2
#define RXFIFO_ADDRESS 0x3F
#define TXFIFO_ADDRESS 0x3F
#define MAX_PACKET_LENGTH 0xFF
#define FIRST_BYTE_WAIT_TIME 2
#define NEXT_BYTE_WAIT_TIME 1

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

bool fifo_wait(size_t ms) {
  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
  while (!hit_timeout(ms, &start)) {
    if (rx_fifo_length() != 0) {
      return true;
    }
  }
  return false;
}

bool radio_change_frequency(uint16_t frequency) {
  // FIXME: am besten sollte man das ohne das MHz reingeben
  set_transceiver_frequency(frequency * MHz);
  return true;
}

bool radio_receive_packet(uint8_t *buffer, unsigned *length) {
  start_receiver_blocking();

  if (!fifo_wait(FIRST_BYTE_WAIT_TIME))
    return false;

  uint8_t packet_length = rx_fifo_pop();
  if (packet_length > *length) {
    fprintf(
        stderr,
        "Buffer with size=%u is too small to receive packet with size=%u.\n",
        *length, packet_length);
    return false;
  }

  uint8_t status[PACKET_STATUS_LENGTH];
  for (size_t i = 0; i < packet_length + PACKET_STATUS_LENGTH; i++) {
    if (!fifo_wait(NEXT_BYTE_WAIT_TIME)) {
      fprintf(stderr, "Expected to receive %u bytes, only got %lu.\n",
              packet_length, i);
      return false;
    }

    if (i < packet_length) {
      buffer[i] = rx_fifo_pop();
    } else {
      status[i - packet_length] = rx_fifo_pop();
    }
  }

  fprintf(stderr,
          "Received packet. Length: %u, RSSI: %i, CRC: %s, Link Quality: %u\n",
          packet_length, status[0], status[1] & (1 << 7) ? "OK" : ":(",
          status[1] & ~(1 << 7));

  *length = packet_length;
  return true;
}

bool radio_send_packet(uint8_t *buffer, unsigned length) {
  if((get_backoff_attempts() > 0) && !check_backoff_timeout()){
    printf("Backoff timeout not expired yet.\n");
    return false;
  }

  if(!collision_detection()){
    printf("First scan unsuccessful. Backing off\n");
    return false;
  }

  for (size_t i = 0; i < length; i++) {
    tx_fifo_push(buffer[i]);
  }
  cc1200_cmd(STX);

  if(!collision_detection()){
    printf("Second scan unsuccessful. Backing off\n");
    return false;
  }

  //TODO in main
  //msg_priority_queue_pop(q);

  reset_backoff_attempts();
  printf("Trasmission successful.\n");
  return true;
}

// NOTE: muss angepasst werden, wenn das Default Interface auf dem Beaglebone
// ein anderes ist
#define ETH_INTERFACE "eth0"
#define MAC_SIZE 6

bool radio_get_id(uint8_t *out) {
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, ETH_INTERFACE, IFNAMSIZ - 1);

  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
    perror("Couldn't read MAC Address from ethernet interface " ETH_INTERFACE);
    return false;
  }

  memcpy(out, ifr.ifr_hwaddr.sa_data, MAC_SIZE);
  close(fd);

  return true;
}
