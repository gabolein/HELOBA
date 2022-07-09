#ifndef MESSAGE_H
#define MESSAGE_H

#include "routing.h"
#include <stdbool.h>
#include <stdint.h>

#define MESSAGE_ACTION_COUNT 4
#define MESSAGE_ACTION_OFFSET 6
#define MESSAGE_ACTION_MASK 0b11000000
typedef enum { DO, DONT, WILL, WONT } message_action_t;

#define MESSAGE_TYPE_COUNT 6
#define MESSAGE_TYPE_OFFSET 0
#define MESSAGE_TYPE_MASK 0b00111111
typedef enum { REPORT, FIND, UPDATE, SWAP, TRANSFER, MUTE } message_type_t;

#define FREQUENCY_ENCODED_SIZE sizeof(uint16_t)
typedef uint16_t frequency_t;

typedef struct {
  message_action_t action;
  message_type_t type;
  routing_id_t sender_id;
  routing_id_t receiver_id;
} message_header_t;

#define OPT_SELF (1 << 7)
#define OPT_PARENT (1 << 6)
#define OPT_LHS (1 << 5)
#define OPT_RHS (1 << 4)

typedef struct {
  uint8_t opt;
  frequency_t self;
  frequency_t parent;
  frequency_t lhs;
  frequency_t rhs;
} local_tree_t;

// NOTE for now, we only expect two frequencies as answer to a DO FIND
// TODO also parent relevant
typedef struct {
  frequency_t lhs;
  frequency_t rhs;
} find_response_t;

typedef struct {
  union {
    routing_id_t to_find;
    find_response_t frequencies;
  };
} find_payload_t;

typedef struct {
  frequency_t old;
  frequency_t updated;
} update_payload_t;

typedef struct {
  local_tree_t tree;
  uint8_t activity_score;
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

#endif
