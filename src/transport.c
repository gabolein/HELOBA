#include "src/transport.h"
#include "lib/datastructures/u8_vector.h"
#include "src/protocol/message.h"
#include "src/protocol/message_parser.h"
#include "src/protocol/routing.h"
#include "src/state.h"
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

#if !defined(VIRTUAL)
  if (!radio_transport_initialize()) {
    return false;
  }
#endif

  return transport_change_frequency(850);
}

bool transport_change_frequency(uint16_t frequency) {
  bool ret;
#if defined(VIRTUAL)
  ret = virtual_change_frequency(frequency);
#else
  ret = radio_change_frequency(frequency);
#endif

  if (!ret) {
    return false;
  }

  gs.frequencies.previous = gs.frequencies.current;
  gs.frequencies.current = frequency;

  return true;
}

bool transport_send_message(message_t *msg, routing_id_t to) {
  msg->header.receiver_id = to;
  msg->header.sender_id = gs.id;

  if (!message_is_valid(msg)) {
    return false;
  }

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

  return message_is_valid(msg) && message_addressed_to(msg, gs.id);
}

bool transport_receive_message_unverified(message_t *msg) {
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
  return message_is_valid(msg);
}

bool transport_get_id(uint8_t *out) {
#if defined(VIRTUAL)
  return virtual_get_id(out);
#else
  return radio_get_id(out);
#endif
}

bool transport_channel_active() {
#if defined(VIRTUAL)
  return virtual_channel_active();
#else
  return radio_channel_active();
#endif
}