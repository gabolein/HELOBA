#define LOG_LEVEL WARNING_LEVEL
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
  struct timespec start_time;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);

  // FIXME: timeout sollte irgendwo mit #define gesetzt werden
  if (!detect_RSSI(100)) {
    return false;
  }

  enable_preamble_detection();
  start_receiver_blocking();

  if (!fifo_wait(RADIO_FIRST_BYTE_WAIT_TIME_MS)) {
    ret = false;
    goto cleanup;
  }

  uint8_t recv_length = rx_fifo_pop();
  if (recv_length > *length) {
    warnln("Buffer with size=%u is too small to receive packet with size=%u.",
           *length, recv_length);
    return false;
  }

  uint8_t status[PACKET_STATUS_LENGTH];
  for (size_t i = 0; i < recv_length + PACKET_STATUS_LENGTH; i++) {
    if (!fifo_wait(RADIO_NEXT_BYTE_WAIT_TIME_MS)) {
      warnln("Expected to receive %u bytes, only got %lu.", recv_length, i);
      ret = false;
      goto cleanup;
    }

    if (i < recv_length) {
      buffer[i] = rx_fifo_pop();
    } else {
      status[i - recv_length] = rx_fifo_pop();
    }
  }

  // NOTE: sollte hier auf CRC geprüft werden oder wird das Paket in dem Fall
  // überhaupt nicht erst angenommen?
  dbgln("Received message. Length: %u, RSSI: %i, CRC: %s, Link Quality: %u",
        recv_length, status[0], status[1] & (1 << 7) ? "OK" : ":(",
        status[1] & ~(1 << 7));

  *length = recv_length + 1;
  ret = true;

cleanup:
  disable_preamble_detection();
  // NOTE: es kann sein, dass Flushen einer leeren FIFO zu Fehlern führt.
  cc1200_cmd(SIDLE);
  cc1200_cmd(SFRX);
  return ret;
}

bool radio_send_packet(uint8_t *buffer, unsigned length) {

  struct timespec start_time;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);

  while (!hit_timeout(RADIO_BACKOFF_TIMEOUT_MS, &start_time)) {
    if ((get_backoff_attempts() > 0) && !check_backoff_timeout()) {
      dbgln("Backoff timeout not expired yet.");
      continue;
    }

    if (!collision_detection()) {
      dbgln("First scan unsuccessful. Backing off");
      continue;
    }

    for (size_t i = 0; i < length; i++) {
      tx_fifo_push(buffer[i]);
    }
    cc1200_cmd(STX);

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

  return false;
}

bool radio_transport_initialize() {
  if (spi_init() != 0) {
    warnln("SPI initialization failed!");
    return false;
  }

  cc1200_cmd(SRES);
  write_default_register_configuration();
  cc1200_cmd(SNOP);
  dbgln("CC1200 Status: %s", get_status_cc1200_str());
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

// FIXME: Hier fehlt noch ein Haufen Zeug:
// 1. Warten bis im IDLE Mode
// 2. In Receive Mode schalten
// 3. detect_RSSI() aufrufen
// 4. Zurück in IDLE Mode schalten
bool radio_channel_active(unsigned timeout_ms) {
  return detect_RSSI(timeout_ms);
}
