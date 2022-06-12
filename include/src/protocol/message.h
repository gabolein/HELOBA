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

#define HEADER_ENCODED_MIN_SIZE                                                \
  (sizeof(uint8_t) + 2 * ROUTING_ID_ENCODED_MIN_SIZE)

typedef struct {
  message_action_t action;
  message_type_t type;
  routing_id_t sender_id;
  routing_id_t receiver_id;
} message_header_t;

typedef struct {
  routing_id_t to_find;
} find_payload_t;

#define OPTMASK_ENCODED_SIZE sizeof(uint8_t)
#define OPT_PARENT_SET (1 << 7)
#define OPT_LHS_SET (1 << 6)
#define OPT_RHS_SET (1 << 5)

typedef struct {
  uint8_t optmask;
  frequency_t parent;
  frequency_t lhs;
  frequency_t rhs;
} update_payload_t;

typedef struct {
  frequency_t source;
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

#endif