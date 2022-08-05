#define LOG_LEVEL DEBUG_LEVEL
#define LOG_LABEL "Radio Transport"

#include "src/beaglebone/radio_transport.h"
#include "lib/logger.h"
#include "lib/time_util.h"
#include "src/beaglebone/backoff.h"
#include "src/beaglebone/frequency.h"
#include "src/beaglebone/registers.h"
#include "src/beaglebone/rssi.h"
#include "src/config.h"
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
  bool ret = false;

  start_receiver_blocking();
  // FIXME: timeout sollte irgendwo mit #define gesetzt werden
  if (!detect_RSSI(RADIO_RECV_BLOCK_WAIT_TIME_MS)) {
    cc1200_cmd(SIDLE);
    return false;
  }

  enable_preamble_detection();
  dbgln("Detected RSSI, starting reception of packet.");
  if (!fifo_wait(RADIO_FIRST_BYTE_WAIT_TIME_MS)) {
    warnln("Didn't receive anything after preamble.");
    ret = false;
    goto cleanup;
  }

  buffer[0] = rx_fifo_pop();
  if (buffer[0] + 1 > *length) {
    warnln("Buffer with size=%u is too small to receive packet with size=%u.",
           *length, buffer[0] + 1);
    return false;
  }

  uint8_t status[PACKET_STATUS_LENGTH];
  for (size_t i = 0; i < buffer[0] + PACKET_STATUS_LENGTH; i++) {
    if (!fifo_wait(RADIO_NEXT_BYTE_WAIT_TIME_MS)) {
      warnln("Expected to receive %u more bytes, only got %lu.", buffer[0], i);
      ret = false;
      goto cleanup;
    }

    if (i < buffer[0]) {
      buffer[i + 1] = rx_fifo_pop();
    } else {
      status[i - buffer[0]] = rx_fifo_pop();
    }
  }

  // NOTE: sollte hier auf CRC geprüft werden oder wird das Paket in dem Fall
  // überhaupt nicht erst angenommen?
  dbgln("Received message. Length: %u, RSSI: %d, CRC: %s, Link Quality: %u",
        buffer[0] + 1, status[0], status[1] & (1 << 7) ? "OK" : ":(",
        status[1] & ~(1 << 7));

  if (!(status[1] & (1 << 7))) {
      ret = false;
  } else {
    *length = buffer[0] + 1;
    ret = true;
  }

cleanup:
  disable_preamble_detection();
  // NOTE: es kann sein, dass Flushen einer leeren FIFO zu Fehlern führt.
  dbgln("Flushing FIFO");
  cc1200_cmd(SFRX);
  cc1200_cmd(SIDLE);
  do {
    cc1200_cmd(SNOP);
  } while (get_status_cc1200() != 0);
  return ret;
}

bool radio_send_packet(uint8_t *buffer, unsigned length) {
  struct timespec start_time;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);

  while (!hit_timeout(RADIO_BACKOFF_TIMEOUT_MS, &start_time)) {
    if ((get_backoff_attempts() > 0) && !check_backoff_timeout()) {
      continue;
    }

    if (!collision_detection()) {
      dbgln("First scan unsuccessful. Backing off");
      continue;
    }

    for (size_t i = 0; i < length; i++) {
      tx_fifo_push(buffer[i]);
    }
    // NOTE blocking? Would we hear our own RSSI?
    cc1200_cmd(STX);

    do {
      cc1200_cmd(SNOP);
    } while (get_status_cc1200() != 0);

    // NOTE: sind wir hier sicher, dass unsere gesendete Nachricht auf jeden
    // Fall nicht angekommen ist?
    if (!collision_detection()) {
      dbgln("Second scan unsuccessful. Backing off");
      continue;
    }

    reset_backoff_attempts();
    dbgln("Transmission successful.");

    return true;
  }

  warnln("Too many backoffs. Aborting sending operation");
  return false;
}

bool radio_transport_initialize() {
  if (spi_init() != 0) {
    warnln("SPI initialization failed!");
    return false;
  }

  cc1200_cmd(SRES);
  write_default_register_configuration();
  disable_preamble_detection();
  cc1200_cmd(SNOP);
  dbgln("CC1200 Status: %s", get_status_cc1200_str());

  int8_t rssi_threshold;
  if (!calculate_RSSI_threshold(&rssi_threshold)) {
    warnln("Couldn't compute RSSI threshold.");
    return false;
  }
  set_rssi_threshold(rssi_threshold);
  return true;
}

// NOTE: muss angepasst werden, wenn das Default Interface auf dem Beaglebone
// ein anderes ist
#define ETH_INTERFACE "eth0"
#define MAC_SIZE 6

bool radio_get_id(uint8_t out[MAC_SIZE]) {
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

bool radio_channel_active(unsigned timeout_ms) {
  start_receiver_blocking();
  bool ret = detect_RSSI(timeout_ms);
  cc1200_cmd(SIDLE);
  return ret;
}
