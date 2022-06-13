#include "src/transport.h"

#if defined(VIRTUAL)
#include "src/state.h"
#include "src/virtual_transport.h"
#else
#include "src/radio_transport.h"
#endif

bool change_frequency(uint16_t frequency) {
#if defined(VIRTUAL)
  // FIXME: Weg finden, keine virtid angeben zu müssen.
  return virtual_change_frequency(frequency, state.virtid);
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

bool receive_packet(uint8_t *buffer, unsigned *length) {
#if defined(VIRTUAL)
  return virtual_receive_packet(buffer, length);
#else
  return radio_receive_packet(buffer, length);
#endif
}
