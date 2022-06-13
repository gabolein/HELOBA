#include "src/transport.h"

#if defined(VIRTUAL)
#include "src/virtual_transport.h"
#else
#include "src/radio_transport.h"
#endif

bool change_frequency(uint16_t frequency) {
#if defined(VIRTUAL)
  // FIXME: Weg finden, keine virtid angeben zu müssen. Das macht die
  // Abstraktion kaputt, weil die generelle Funktion deswegen auch eine virtid
  // annehmen muss. Als temporären Fix gehen wir einfach von virtid 0 aus.
  return virtual_change_frequency(frequency, 0);
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
