#include "src/protocol/message_formatter.h"
#include "lib/datastructures/generic/generic_vector.h"
#include <stdio.h>

MAKE_SPECIFIC_VECTOR_HEADER(char, char) // Binks!
MAKE_SPECIFIC_VECTOR_SOURCE(char, char)

void append_helper(char_vector_t *v, char *str) {
  for (unsigned i = 0; i < strlen(str); i++) {
    char_vector_append(v, str[i]);
  }
}

static char_vector_t *action_vec;
static char_vector_t *type_vec;
static char_vector_t *routing_id_vec;
static char_vector_t *num_vec;
static char_vector_t *message_vec;

void formatter_initialize() {
  action_vec = char_vector_create_with_capacity(8);
  type_vec = char_vector_create_with_capacity(8);
  routing_id_vec = char_vector_create_with_capacity(64);
  num_vec = char_vector_create_with_capacity(32);
  message_vec = char_vector_create_with_capacity(256);
}

void formatter_teardown() {
  char_vector_destroy(action_vec);
  char_vector_destroy(type_vec);
  char_vector_destroy(routing_id_vec);
  char_vector_destroy(num_vec);
  char_vector_destroy(message_vec);
}

char *format_number(char *fmt, unsigned num) {
  assert(fmt != NULL);

  int required_length = snprintf(NULL, 0, fmt, num) + 1;
  assert(required_length >= 0);

  char_vector_ensure_capacity(num_vec, required_length);
  snprintf(num_vec->data, required_length, fmt, num);
  return num_vec->data;
}

char *format_action(message_action_t action) {
  char_vector_clear(action_vec);

  switch (action) {
  case DO:
    append_helper(action_vec, "DO");
    break;
  case DONT:
    append_helper(action_vec, "DONT");
    break;
  case WILL:
    append_helper(action_vec, "WILL");
    break;
  case WONT:
    append_helper(action_vec, "WONT");
    break;
  }

  char_vector_append(action_vec, '\0');
  return action_vec->data;
}

char *format_type(message_type_t type) {
  char_vector_clear(type_vec);

  switch (type) {
  case HINT:
    append_helper(type_vec, "HINT");
    break;
  case FIND:
    append_helper(type_vec, "FIND");
    break;
  case SWAP:
    append_helper(type_vec, "SWAP");
    break;
  case TRANSFER:
    append_helper(type_vec, "TRANSFER");
    break;
  case MIGRATE:
    append_helper(type_vec, "MIGRATE");
    break;
  case SPLIT:
    append_helper(type_vec, "SPLIT");
    break;
  }

  char_vector_append(type_vec, '\0');
  return type_vec->data;
}

char *format_routing_id(routing_id_t id) {
  char_vector_clear(routing_id_vec);
  char_vector_append(routing_id_vec, '<');

  bool printed_once = false;

  for (unsigned i = 0; i < 3; i++) {
    if (id.layer & (1 << i)) {
      if (printed_once) {
        append_helper(routing_id_vec, " | ");
      }

      switch (1 << i) {
      case leader:
        append_helper(routing_id_vec, "Leader");
        break;
      case everyone:
        append_helper(routing_id_vec, "Everyone");
        break;
      case specific:
        append_helper(routing_id_vec, "Specific");
        break;
      }

      printed_once = true;
    }
  }

  if (!(id.layer & specific)) {
    char_vector_append(routing_id_vec, '>');
    char_vector_append(routing_id_vec, '\0');
    return routing_id_vec->data;
  }

  if (printed_once) {
    char_vector_append(routing_id_vec, ' ');
  }

  for (unsigned i = 0; i < MAC_SIZE; i++) {
    if (i != 0) {
      append_helper(routing_id_vec, ":");
    }

    append_helper(routing_id_vec, format_number("%02hhx", id.MAC[i]));
  }

  char_vector_append(routing_id_vec, '>');
  char_vector_append(routing_id_vec, '\0');
  return routing_id_vec->data;
}

char *format_message(message_t *msg) {
  char_vector_clear(message_vec);

  append_helper(message_vec, format_action(msg->header.action));
  char_vector_append(message_vec, ' ');
  append_helper(message_vec, format_type(msg->header.type));
  append_helper(message_vec, " From: ");
  append_helper(message_vec, format_routing_id(msg->header.sender_id));
  append_helper(message_vec, ", To: ");
  append_helper(message_vec, format_routing_id(msg->header.receiver_id));
  append_helper(message_vec, ", Payload: { ");

  switch (msg->header.type) {
  case HINT:
    append_helper(message_vec, "Frequency: ");
    append_helper(message_vec, format_number("%u", msg->payload.hint.hint.f));
    append_helper(message_vec, ", Timedelta: ");
    append_helper(message_vec,
                  format_number("%u", msg->payload.hint.hint.timedelta_us));
    append_helper(message_vec, "us");
    break;
  case FIND:
    append_helper(message_vec, "Wanted: ");
    append_helper(message_vec, format_routing_id(msg->payload.find.to_find));
    break;
  case SWAP:
    append_helper(message_vec, "Target: ");
    append_helper(message_vec, format_number("%u", msg->payload.swap.with));
    append_helper(message_vec, ", Score: ");
    append_helper(message_vec, format_number("%u", msg->payload.swap.score));
    break;
  case TRANSFER:
    append_helper(message_vec, "Target: ");
    append_helper(message_vec, format_number("%u", msg->payload.transfer.to));
    break;
  case MIGRATE:
    // FIXME: richtigen Typ für Migrate
    append_helper(message_vec, "Target: ");
    append_helper(message_vec, format_number("%u", msg->payload.transfer.to));
    break;
  case SPLIT:
    append_helper(message_vec, "Direction: ");
    append_helper(message_vec,
                  msg->payload.split.direction == SPLIT_UP ? "UP" : "DOWN");
    append_helper(message_vec, ", LHS Bound: ");
    append_helper(message_vec, format_routing_id(msg->payload.split.delim1));
    append_helper(message_vec, ", RHS Bound: ");
    append_helper(message_vec, format_routing_id(msg->payload.split.delim2));
    break;
  };

  append_helper(message_vec, " }");
  char_vector_append(message_vec, '\0');
  return message_vec->data;
}
