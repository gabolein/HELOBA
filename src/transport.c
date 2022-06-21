#include "src/transport.h"
#include <stdint.h>

#if defined(VIRTUAL)
#include "src/virtual_transport.h"
#else
#include "src/beaglebone/radio_transport.h"
#endif

bool transport_initialize() {
#if defined(VIRTUAL)
  return virtual_transport_initialize();
#else
  return radio_transport_initialize();
#endif
}

bool change_frequency(uint16_t frequency) {
#if defined(VIRTUAL)
  return virtual_change_frequency(frequency);
#else
  return radio_change_frequency(frequency);
#endif
}

bool send_packet(uint8_t *buffer, unsigned length) {
#if defined(VIRTUAL)
  return virtual_send_packet(buffer, length);
#else
  return radio_send_packet(buffer, length);
#endif
}

bool listen(uint8_t *buffer, unsigned *length, unsigned listen_ms) {
#if defined(VIRTUAL)
  return virtual_listen(buffer, length, listen_ms);
#else
  return radio_listen(buffer, length, listen_ms);
#endif
}

bool receive_packet(uint8_t *buffer, unsigned *length) {
#if defined(VIRTUAL)
  return virtual_receive_packet(buffer, length);
#else
  return radio_receive_packet(buffer, length);
#endif
}

bool get_id(uint8_t *out) {
#if defined(VIRTUAL)
  return virtual_get_id(out);
#else
  return radio_get_id(out);
#endif
}
