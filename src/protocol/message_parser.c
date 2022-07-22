#define LOG_LEVEL DEBUG_LEVEL
#define LOG_LABEL "Message Parser"

#include "lib/datastructures/u8_vector.h"
#include "lib/logger.h"
#include "src/protocol/message.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

void pack_frequency(u8_vector_t *v, frequency_t freq);
void pack_routing_id(u8_vector_t *v, routing_id_t *id);
void pack_header(u8_vector_t *v, message_header_t *header);
void pack_find_payload(u8_vector_t *v, find_payload_t *payload);
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
  if (layer & specific) {
    for (size_t i = 0; i < MAC_SIZE; i++) {
      // TODO: Nachschauen ob das in NW Byteorder sein muss bzw. ob wir das zu
      // diesem Zeitpunkt überhaupt schon brauchen.
      u8_vector_append(v, id->MAC[i]);
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

void pack_hint_payload(u8_vector_t *v, hint_payload_t *payload) {
  pack_frequency(v, payload->hint.f);
  u8_vector_append(v, (payload->hint.timedelta_us >> 24) & 0xff);
  u8_vector_append(v, (payload->hint.timedelta_us >> 16) & 0xff);
  u8_vector_append(v, (payload->hint.timedelta_us >> 8) & 0xff);
  u8_vector_append(v, (payload->hint.timedelta_us >> 0) & 0xff);
}

void pack_find_payload(u8_vector_t *v, find_payload_t *payload) {
  pack_routing_id(v, &payload->to_find);
}

void pack_swap_payload(u8_vector_t *v, swap_payload_t *payload) {
  pack_frequency(v, payload->with);
  u8_vector_append(v, payload->score);
}

void pack_transfer_payload(u8_vector_t *v, transfer_payload_t *payload) {
  pack_frequency(v, payload->to);
}

void pack_split_payload(u8_vector_t *v, split_payload_t *payload) {
  split_direction dir = payload->direction;
  u8_vector_append(v, (uint8_t)dir);
  pack_routing_id(v, &payload->delim1);
  pack_routing_id(v, &payload->delim2);
}

unsigned get_payload_size(message_t *msg) {
  // TODO replace magic numbers
  switch (msg->header.type) {
  case HINT:
    // Das ist ein Level zu viel
    return sizeof(msg->payload.hint.hint.f) +
           sizeof(msg->payload.hint.hint.timedelta_us);
  case FIND:
    return msg->payload.find.to_find.layer & specific ? 7 : 1;
  case SWAP:
    return 2 + 1;
  case MIGRATE:
    return 2;
  case TRANSFER:
    return 2;
  case SPLIT:
    return 1 + 7 + 7;
  default:
    warnln("get_payload_size: case not handled");
    return 0;
  }
}

void pack_message_length(u8_vector_t *v, message_t *msg) {

  uint8_t message_length = 0;
  // add action and type
  message_length += 1;
  // add sender routing_id
  message_length += 7;
  // add recv sender id
  message_length += 1;
  if (msg->header.receiver_id.layer & specific) {
    message_length += 6;
  }

  // add payload size
  message_length += get_payload_size(msg);
  u8_vector_append(v, message_length);
  pack_header(v, &msg->header);
}

void pack_message(u8_vector_t *v, message_t *msg) {
  assert(u8_vector_size(v) == 0);

  pack_message_length(v, msg);

  switch (msg->header.type) {
  case HINT:
    pack_hint_payload(v, &msg->payload.hint);
    break;
  case FIND:
    pack_find_payload(v, &msg->payload.find);
    break;
  case SWAP:
    pack_swap_payload(v, &msg->payload.swap);
    break;
  case TRANSFER:
    pack_transfer_payload(v, &msg->payload.transfer);
    break;
  case MIGRATE:
    pack_transfer_payload(v, &msg->payload.transfer);
    break;
  case SPLIT:
    pack_split_payload(v, &msg->payload.split);
  default:
    break;
  };

  dbgln("Packed message of size = %u bytes", u8_vector_size(v));
  assert(u8_vector_at(v, 0) == u8_vector_size(v) - 1);
}

frequency_t unpack_frequency(uint8_t *buffer, unsigned length,
                             unsigned *decoded) {
  assert(length - *decoded >= FREQUENCY_ENCODED_SIZE);

  uint16_t d = buffer[*decoded] << 8 | buffer[*decoded + 1];
  *decoded += 2;
  return (frequency_t)d;
}

routing_id_t unpack_routing_id(uint8_t *buffer, unsigned length,
                               unsigned *decoded) {
  assert(*decoded <= length);
  assert(length - *decoded >= sizeof(uint8_t));
  if (buffer[*decoded] & specific)
    assert(length - *decoded >= sizeof(uint8_t) + MAC_SIZE);

  routing_id_t d;
  memset(&d, 0, sizeof(routing_id_t));
  d.layer = buffer[*decoded];
  (*decoded)++;

  if (d.layer & specific) {
    for (size_t i = 0; i < MAC_SIZE; i++) {
      d.MAC[i] = buffer[*decoded];
      (*decoded)++;
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

hint_payload_t unpack_hint_payload(uint8_t *buffer, unsigned length,
                                   unsigned *decoded) {
  hint_payload_t d;
  memset(&d.hint, 0, sizeof(d.hint));
  d.hint.f = unpack_frequency(buffer, length, decoded);

  assert(*decoded <= length);
  assert(length - *decoded >= sizeof(d.hint.timedelta_us));

  for (unsigned i = 0; i < sizeof(d.hint.timedelta_us); i++) {
    d.hint.timedelta_us <<= 8;
    d.hint.timedelta_us |= buffer[*decoded];
    (*decoded)++;
  }

  return d;
}

find_payload_t unpack_find_payload(uint8_t *buffer, unsigned length,
                                   unsigned *decoded) {
  find_payload_t d;
  d.to_find = unpack_routing_id(buffer, length, decoded);
  return d;
}

swap_payload_t unpack_swap_payload(uint8_t *buffer, unsigned length,
                                   unsigned *decoded) {
  swap_payload_t d;

  d.with = unpack_frequency(buffer, length, decoded);

  assert(length - *decoded >= sizeof(uint8_t));
  d.score = buffer[*decoded];
  (*decoded)++;

  return d;
}

transfer_payload_t unpack_transfer_payload(uint8_t *buffer, unsigned length,
                                           unsigned *decoded) {
  transfer_payload_t d;
  d.to = unpack_frequency(buffer, length, decoded);
  return d;
}

split_payload_t unpack_split_payload(uint8_t *buffer, unsigned length,
                                     unsigned *decoded) {
  split_payload_t d;

  d.direction = buffer[*decoded];
  (*decoded)++;

  d.delim1 = unpack_routing_id(buffer, length, decoded);
  d.delim2 = unpack_routing_id(buffer, length, decoded);

  return d;
}

message_t unpack_message(uint8_t *buffer, unsigned length) {
  message_t d;
  unsigned decoded = 1;

  d.header = unpack_header(buffer, length, &decoded);

  switch (d.header.type) {
  case HINT:
    d.payload.hint = unpack_hint_payload(buffer, length, &decoded);
    break;
  case FIND:
    d.payload.find = unpack_find_payload(buffer, length, &decoded);
    break;
  case SWAP:
    d.payload.swap = unpack_swap_payload(buffer, length, &decoded);
    break;
  case TRANSFER:
    d.payload.transfer = unpack_transfer_payload(buffer, length, &decoded);
    break;
  case MIGRATE:
    d.payload.transfer = unpack_transfer_payload(buffer, length, &decoded);
    break;
  case SPLIT:
    d.payload.split = unpack_split_payload(buffer, length, &decoded);
  default:
    break;
  }

  // NOTE Es muss nicht alles decoded werden
  /*assert(decoded == length);*/
  return d;
}
