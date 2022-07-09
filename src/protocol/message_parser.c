
#include "lib/datastructures/u8_vector.h"
#include "src/protocol/message.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

void pack_frequency(u8_vector_t *v, frequency_t freq);
void pack_routing_id(u8_vector_t *v, routing_id_t *id);
void pack_header(u8_vector_t *v, message_header_t *header);
void pack_find_payload(u8_vector_t *v, find_payload_t *payload);
void pack_update_payload(u8_vector_t *v, update_payload_t *payload);
void pack_swap_payload(u8_vector_t *v, swap_payload_t *payload);
void pack_transfer_payload(u8_vector_t *v, transfer_payload_t *payload);

frequency_t unpack_frequency(uint8_t *buffer, unsigned length,
                             unsigned *decoded);
message_header_t unpack_header(uint8_t *buffer, unsigned length,
                               unsigned *decoded);
routing_id_t unpack_routing_id(uint8_t *buffer, unsigned length,
                               unsigned *decoded);
find_payload_t unpack_find_payload(uint8_t *buffer, unsigned length,
                                   unsigned *decoded);
update_payload_t unpack_update_payload(uint8_t *buffer, unsigned length,
                                       unsigned *decoded);
swap_payload_t unpack_swap_payload(uint8_t *buffer, unsigned length,
                                   unsigned *decoded);
transfer_payload_t unpack_transfer_payload(uint8_t *buffer, unsigned length,
                                           unsigned *decoded);

void pack_frequency(u8_vector_t *v, frequency_t freq) {
  u8_vector_append(v, ((uint16_t)freq & 0xFF00) >> 8);
  u8_vector_append(v, ((uint16_t)freq & 0x00FF) >> 0);
}

void pack_routing_id(u8_vector_t *v, routing_id_t *id) {
  routing_layer_t layer = id->layer;
  // NOTE: Wir nehmen hier an, dass das in einen uint8_t passt.
  // Das kann in der Zukunft mit einem switch Statement sicherer gemacht werden,
  // falls wir noch weitere Layer hinzufügen (unwahrscheinlich).
  u8_vector_append(v, (uint8_t)layer);

  // NOTE: Sollte bei leader auch die MAC Adresse gesendet werden?
  if (layer == specific) {
    for (size_t i = 0; i < MAC_SIZE; i++) {
      // TODO: Nachschauen ob das in NW Byteorder sein muss bzw. ob wir das zu
      // diesem Zeitpunkt überhaupt schon brauchen.
      u8_vector_append(v, id->optional_MAC[i]);
    }
  }
}

void pack_header(u8_vector_t *v, message_header_t *header) {
  uint8_t pt = 0;

  pt |= header->action << MESSAGE_ACTION_OFFSET;
  pt |= header->type << MESSAGE_TYPE_OFFSET;

  u8_vector_append(v, pt);

  pack_routing_id(v, &header->sender_id);
  pack_routing_id(v, &header->receiver_id);
}

void pack_find_payload(u8_vector_t *v, find_payload_t *payload) {
  pack_routing_id(v, &payload->to_find);
}

void pack_update_payload(u8_vector_t *v, update_payload_t *payload) {
  pack_frequency(v, payload->old);
  pack_frequency(v, payload->updated);
}

void pack_swap_payload(u8_vector_t *v, swap_payload_t *payload) {
  pack_frequency(v, payload->source);
  u8_vector_append(v, payload->activity_score);
}

void pack_transfer_payload(u8_vector_t *v, transfer_payload_t *payload) {
  pack_frequency(v, payload->to);
}

void pack_message(u8_vector_t *v, message_t *msg) {
  assert(u8_vector_size(v) == 0);

  pack_header(v, &msg->header);

  switch (msg->header.type) {
  case FIND:
    pack_find_payload(v, &msg->payload.find);
    break;
  case UPDATE:
    pack_update_payload(v, &msg->payload.update);
    break;
  case SWAP:
    pack_swap_payload(v, &msg->payload.swap);
    break;
  case TRANSFER:
    pack_transfer_payload(v, &msg->payload.transfer);
    break;
  default:
    break;
  };
}

frequency_t unpack_frequency(uint8_t *buffer, unsigned length,
                             unsigned *decoded) {
  assert(length - *decoded >= FREQUENCY_ENCODED_SIZE);

  uint16_t d = buffer[(*decoded)++] << 8 | buffer[(*decoded)++];
  return (frequency_t)d;
}

routing_id_t unpack_routing_id(uint8_t *buffer, unsigned length,
                               unsigned *decoded) {
  assert(*decoded <= length);
  assert(length - *decoded >= sizeof(uint8_t));
  if (buffer[*decoded] == specific)
    assert(length - *decoded >= sizeof(uint8_t) + MAC_SIZE);

  routing_id_t d;
  d.layer = buffer[*decoded++];

  if (d.layer == specific) {
    for (size_t i = 0; i < MAC_SIZE; i++) {
      d.optional_MAC[i] = buffer[*decoded++];
    }
  }

  return d;
}

message_header_t unpack_header(uint8_t *buffer, unsigned length,
                               unsigned *decoded) {
  assert(*decoded <= length);
  assert(length - *decoded >= sizeof(uint8_t));

  message_header_t d;
  d.action = (buffer[*decoded] & MESSAGE_ACTION_MASK) >> MESSAGE_ACTION_OFFSET;
  d.type = (buffer[*decoded] & MESSAGE_TYPE_MASK) >> MESSAGE_TYPE_OFFSET;
  (*decoded)++;

  d.sender_id = unpack_routing_id(buffer, length, decoded);
  d.receiver_id = unpack_routing_id(buffer, length, decoded);

  return d;
}

find_payload_t unpack_find_payload(uint8_t *buffer, unsigned length,
                                   unsigned *decoded) {
  find_payload_t d;
  d.to_find = unpack_routing_id(buffer, length, decoded);
  return d;
}

update_payload_t unpack_update_payload(uint8_t *buffer, unsigned length,
                                       unsigned *decoded) {
  update_payload_t d;
  d.old = unpack_frequency(buffer, length, decoded);
  d.updated = unpack_frequency(buffer, length, decoded);
  return d;
}

swap_payload_t unpack_swap_payload(uint8_t *buffer, unsigned length,
                                   unsigned *decoded) {
  swap_payload_t d;

  d.source = unpack_frequency(buffer, length, decoded);

  assert(length - *decoded >= sizeof(uint8_t));
  d.activity_score = buffer[*decoded++];

  return d;
}

transfer_payload_t unpack_transfer_payload(uint8_t *buffer, unsigned length,
                                           unsigned *decoded) {
  transfer_payload_t d;
  d.to = unpack_frequency(buffer, length, decoded);
  return d;
}

message_t unpack_message(uint8_t *buffer, unsigned length) {
  message_t d;
  unsigned decoded = 0;

  d.header = unpack_header(buffer, length, &decoded);

  switch (d.header.type) {
  case FIND:
    d.payload.find = unpack_find_payload(buffer, length, &decoded);
    break;
  case UPDATE:
    d.payload.update = unpack_update_payload(buffer, length, &decoded);
    break;
  case SWAP:
    d.payload.swap = unpack_swap_payload(buffer, length, &decoded);
    break;
  case TRANSFER:
    d.payload.transfer = unpack_transfer_payload(buffer, length, &decoded);
    break;
  default:
    break;
  }

  assert(decoded == length);
  return d;
}