#ifndef MESSAGE_H
#define MESSAGE_H

#include "routing.h"
#include <stdbool.h>
#include <stdint.h>

#define MESSAGE_ACTION_COUNT 4
#define MESSAGE_ACTION_OFFSET 6
#define MESSAGE_ACTION_MASK 0b11000000
typedef enum { DO, DONT, WILL, WONT } message_action_t;

#define MESSAGE_TYPE_COUNT 5
#define MESSAGE_TYPE_OFFSET 0
#define MESSAGE_TYPE_MASK 0b00111111
typedef enum { FIND, SWAP, TRANSFER, MUTE, MIGRATE } message_type_t;

#define FREQUENCY_ENCODED_SIZE sizeof(uint16_t)
typedef uint16_t frequency_t;

typedef struct {
  message_action_t action;
  message_type_t type;
  routing_id_t sender_id;
  routing_id_t receiver_id;
} message_header_t;

typedef struct {
  // FIXME: unions sollten zu einem Minimum gehalten werden
  union {
    routing_id_t to_find;
    frequency_t cached;
  };
} find_payload_t;

typedef struct {
  frequency_t old;
  frequency_t updated;
} update_payload_t;

typedef struct {
  frequency_t with;
  uint8_t score;
} swap_payload_t;

typedef struct {
  frequency_t to;
} transfer_payload_t;

typedef struct {
  message_header_t header;
  union {
    find_payload_t find;
    update_payload_t update;
    swap_payload_t swap;
    transfer_payload_t transfer;
  } payload;
} message_t;

message_action_t message_action(message_t *msg);
message_type_t message_type(message_t *msg);
bool message_from_leader(message_t *msg);
bool message_is_valid(message_t *msg);
bool message_addressed_to(message_t *msg, routing_id_t id);
message_t message_create(message_action_t action, message_type_t type);

#endif
