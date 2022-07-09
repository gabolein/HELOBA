#include "src/transport.h"
#include "lib/datastructures/u8_vector.h"
#include "src/protocol/message.h"
#include "src/protocol/message_parser.h"
#include <stdint.h>

#if defined(VIRTUAL)
#include "src/virtual_transport.h"
#else
#include "src/beaglebone/radio_transport.h"
#endif

// FIXME: entweder beides buffer oder beides vector, aber nicht gemischt.
uint8_t recv_buffer[UINT8_MAX];
u8_vector_t *send_vec;

bool transport_initialize() {
  send_vec = u8_vector_create();

#if defined(VIRTUAL)
  return virtual_change_frequency(850);
#else
  if (!radio_transport_initialize()) {
    return false;
  }
  return radio_change_frequency(850);
#endif
}

bool transport_change_frequency(uint16_t frequency) {
#if defined(VIRTUAL)
  return virtual_change_frequency(frequency);
#else
  return radio_change_frequency(frequency);
#endif
}

bool transport_send_message(message_t *msg) {
  pack_message(send_vec, msg);

#if defined(VIRTUAL)
  bool ret = virtual_send_packet(send_vec->data, u8_vector_size(send_vec));
#else
  bool ret = radio_send_packet(send_vec->data, u8_vector_size(send_vec));
#endif

  u8_vector_clear(send_vec);
  return ret;
}

bool transport_receive_message(message_t *msg) {
  unsigned length = sizeof(recv_buffer);
#if defined(VIRTUAL)
  bool ret = virtual_receive_packet(recv_buffer, &length);
#else
  bool ret = radio_receive_packet(recv_buffer, &length);
#endif

  if (ret == false) {
    return false;
  }

  *msg = unpack_message(recv_buffer, length);
  return true;
}

bool transport_get_id(uint8_t *out) {
#if defined(VIRTUAL)
  return virtual_get_id(out);
#else
  return radio_get_id(out);
#endif
}
