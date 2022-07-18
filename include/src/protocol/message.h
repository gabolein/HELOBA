#ifndef MESSAGE_H
#define MESSAGE_H

#include "lib/datastructures/generic/generic_vector.h"
#include <stdbool.h>
#include <stdint.h>

#define MAX_MSG_LEN 255

#define MESSAGE_ACTION_COUNT 4
#define MESSAGE_ACTION_OFFSET 6
#define MESSAGE_ACTION_MASK 0b11000000
typedef enum { DO, DONT, WILL, WONT } message_action_t;

#define MESSAGE_TYPE_COUNT 6
#define MESSAGE_TYPE_OFFSET 0
#define MESSAGE_TYPE_MASK 0b00111111
typedef enum { HINT, FIND, SWAP, TRANSFER, MIGRATE, SPLIT } message_type_t;

#define MAC_SIZE 6
#define ROUTING_ID_ENCODED_MIN_SIZE 1
#define ROUTING_ID_ENCODED_MAX_SIZE 6

// TODO: besseren Namen für die ID finden, vielleicht network_id_t?
typedef enum {
  everyone = 1 << 0,
  leader = 1 << 1,
  specific = 1 << 2,
} routing_layer_t;

typedef struct {
  routing_layer_t layer;
  uint8_t MAC[MAC_SIZE];
} routing_id_t;

#define FREQUENCY_ENCODED_SIZE sizeof(uint16_t)
typedef uint16_t frequency_t;

typedef struct {
  message_action_t action;
  message_type_t type;
  routing_id_t sender_id;
  routing_id_t receiver_id;
} message_header_t;

// Verarbeiteter Cache Eintrag, der als Antwort auf DO FIND an andere Nodes
// geschickt werden kann.
//
// f: Die Frequenz, auf der der Node mit der angefragten ID sein könnte
// timedelta: Alter des Cacheeintrags in Mikrosekunden. Damit ist das maximale
// Alter eines Cache Eintrags ~70 Minuten.
// NOTE: Eigentlich sollte die Definition in cache.h stehen, wegen Circular
// Includes packen wir es erstmal hierhin. Sollte wieder zurück geschoben
// werden, sobald wir für message.h eine modulare Struktur wie bei den Handlers
// haben.
typedef struct {
  frequency_t f;
  unsigned timedelta_us;
} cache_hint_t;

typedef struct {
  cache_hint_t hint;
} hint_payload_t;

typedef struct {
  routing_id_t to_find;
} find_payload_t;

typedef struct {
  frequency_t with;
  uint8_t score;
} swap_payload_t;

typedef struct {
  frequency_t to;
} transfer_payload_t;

typedef struct {
  routing_id_t delim1;
  routing_id_t delim2;
} split_payload_t;

typedef struct {
  message_header_t header;
  union {
    hint_payload_t hint;
    find_payload_t find;
    swap_payload_t swap;
    transfer_payload_t transfer;
    split_payload_t split;
  } payload;
} message_t;

MAKE_SPECIFIC_VECTOR_HEADER(message_t, message)

message_action_t message_action(message_t *msg);
message_type_t message_type(message_t *msg);
bool message_from_leader(message_t *msg);
bool message_is_valid(message_t *msg);
bool routing_id_equal(routing_id_t id1, routing_id_t id2);
bool message_addressed_to(message_t *msg, routing_id_t id);
message_t message_create(message_action_t action, message_type_t type);
routing_id_t routing_id_create(routing_layer_t layer, uint8_t *MAC);
void message_dbgln(message_t *msg);

#endif
