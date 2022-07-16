#define LOG_LEVEL DEBUG_LEVEL
#define LOG_LABEL "Message"

#include "src/protocol/message.h"
#include "lib/datastructures/generic/generic_priority_queue.h"
#include "lib/datastructures/generic/generic_vector.h"
#include "lib/logger.h"
#include "src/protocol/routing.h"
#include "src/state.h"
#include "src/transport.h"
#include <stdio.h>

MAKE_SPECIFIC_VECTOR_SOURCE(message_t, message)

static bool message_allowlist[MESSAGE_ACTION_COUNT][MESSAGE_TYPE_COUNT] = {
    [DO][MUTE] = true,
    [DONT][MUTE] = true,
    [DO][SWAP] = true,
    [WILL][SWAP] = true,
    [WONT][SWAP] = true,
    // NOTE: WONT TRANSFER wäre eigentlich auch ganz hilfreich, um TRANSFER
    // zu einer schon zu großen Gruppe zu verhindern. In dem Fall könnte die
    // Gruppe nach dem TRANSFER aber auch einfach in 2 geteilt werden.
    [DO][TRANSFER] = true,
    [WILL][TRANSFER] = true,
    [DO][FIND] = true,
    [WILL][FIND] = true};

inline message_action_t message_action(message_t *msg) {
  return msg->header.action;
}

inline message_type_t message_type(message_t *msg) { return msg->header.type; }

inline bool message_from_leader(message_t *msg) {
  return msg->header.sender_id.layer == leader;
}

bool message_is_command(message_t *msg) {
  return message_action(msg) == DO || message_action(msg) == DONT;
}

bool message_is_valid(message_t *msg) {
  if (msg == NULL) {
    warnln("Message is NULL.");
    return false;
  }

  if (message_allowlist[message_action(msg)][message_type(msg)] == false) {
    warnln("Message has invalid ACTION/TYPE combination.");
    return false;
  }

  if (!(msg->header.sender_id.layer & specific)) {
    warnln("Sender ID Layer = %#0x does not have 'specific' bit set",
           msg->header.sender_id.layer);
    return false;
  }

  return true;
}

bool routing_id_equal(routing_id_t id1, routing_id_t id2) {
  return (id1.layer & specific) && (id2.layer & specific) &&
         memcmp(id1.MAC, id2.MAC, MAC_SIZE) == 0;
}

bool message_addressed_to(message_t *msg, routing_id_t id) {
  // NOTE: wird für den VIRTUAL Modus gebraucht, weil wir nach dem Abschicken
  // unsere eigenen UDP Nachrichten empfangen.
  if (routing_id_equal(msg->header.sender_id, id)) {
    return false;
  }

  if (msg->header.receiver_id.layer & everyone) {
    return true;
  }

  if ((msg->header.receiver_id.layer & leader) == (id.layer & leader)) {
    return true;
  }

  if (routing_id_equal(msg->header.receiver_id, id)) {
    return true;
  }

  return false;
}

message_t message_create(message_action_t action, message_type_t type) {
  assert(message_allowlist[action][type]);

  message_t msg;
  memset(&msg, 0, sizeof(msg));
  msg.header.action = action;
  msg.header.type = type;

  return msg;
}

MAKE_SPECIFIC_VECTOR_HEADER(char, char) // Binks!
MAKE_SPECIFIC_VECTOR_SOURCE(char, char)

void repr_append(char *str, char_vector_t *repr) {
  for (unsigned i = 0; i < strlen(str); i++) {
    char_vector_append(repr, str[i]);
  }
}

void message_format_action(message_action_t action, char_vector_t *repr) {
  repr_append("Action: ", repr);

  switch (action) {
  case DO:
    repr_append("DO", repr);
    break;
  case DONT:
    repr_append("DONT", repr);
    break;
  case WILL:
    repr_append("WILL", repr);
    break;
  case WONT:
    repr_append("WONT", repr);
    break;
  }
}

void message_format_type(message_type_t type, char_vector_t *repr) {
  repr_append("Type: ", repr);

  switch (type) {
  case FIND:
    repr_append("FIND", repr);
    break;
  case SWAP:
    repr_append("SWAP", repr);
    break;
  case TRANSFER:
    repr_append("TRANSFER", repr);
    break;
  case MUTE:
    repr_append("MUTE", repr);
    break;
  case MIGRATE:
    repr_append("MIGRATE", repr);
    break;
  case SPLIT:
    repr_append("SPLIT", repr);
    break;
  }
}

void message_format_id(routing_id_t id, char_vector_t *repr) {
  bool printed_once = false;

  for (unsigned i = 0; i < 3; i++) {
    if (id.layer & (1 << i)) {
      if (printed_once) {
        repr_append(" | ", repr);
      }

      switch (1 << i) {
      case leader:
        repr_append("Leader", repr);
        break;
      case everyone:
        repr_append("Everyone", repr);
        break;
      case specific:
        repr_append("Specific", repr);
        break;
      }

      printed_once = true;
    }
  }

  if (printed_once) {
    repr_append(", ", repr);
  }

  repr_append("MAC ", repr);
  if (!(id.layer & specific)) {
    repr_append("<empty>", repr);
    return;
  }

  for (unsigned i = 0; i < MAC_SIZE; i++) {
    if (i != 0) {
      repr_append(":", repr);
    }

    char hex[3];
    snprintf(hex, 3, "%hhx", id.MAC[i]);
    repr_append(hex, repr);
  }
}

#define U16_PRINT_WIDTH 6

void message_format_frequency(frequency_t f, char_vector_t *repr) {
  char dec[U16_PRINT_WIDTH];
  snprintf(dec, U16_PRINT_WIDTH, "%hu", f);
  repr_append(dec, repr);
}

#define U8_PRINT_WIDTH 3

void message_format_u8(uint8_t num, char_vector_t *repr) {
  char dec[U8_PRINT_WIDTH];
  snprintf(dec, U8_PRINT_WIDTH, "%hhu", num);
  repr_append(dec, repr);
}

void message_dbgln(message_t *msg) {
#if LOG_LEVEL == DEBUG_LEVEL
  char_vector_t *repr = char_vector_create();

  char_vector_append(repr, '\n');
  message_format_action(msg->header.action, repr);
  repr_append("\n", repr);
  message_format_type(msg->header.type, repr);
  repr_append("\nFrom: ", repr);
  message_format_id(msg->header.sender_id, repr);
  repr_append("\nTo: ", repr);
  message_format_id(msg->header.receiver_id, repr);
  repr_append("\n", repr);

  switch (msg->header.type) {
  case FIND:
    // FIXME: Lösung für dieses Union Problem finden, im Moment geben wir
    // einfach beide aus, ohne zu wissen welches valide ist...
    repr_append("Wanted: ", repr);
    message_format_id(msg->payload.find.to_find, repr);
    repr_append("\nFrequency: ", repr);
    message_format_frequency(msg->payload.find.cached, repr);
    break;
  case SWAP:
    repr_append("Target: ", repr);
    message_format_frequency(msg->payload.swap.with, repr);
    repr_append("\nScore: ", repr);
    message_format_u8(msg->payload.swap.score, repr);
    break;
  case TRANSFER:
    repr_append("Target: ", repr);
    message_format_frequency(msg->payload.transfer.to, repr);
    break;
  case MUTE:
    repr_append("<empty>", repr);
    break;
  case MIGRATE:
    // FIXME: richtigen Typ für Migrate
    repr_append("Target: ", repr);
    message_format_frequency(msg->payload.transfer.to, repr);
    break;
  case SPLIT:
    repr_append("First Delimeter: ", repr);
    message_format_id(msg->payload.split.delim1, repr);
    repr_append("\nSecond Delimeter: ", repr);
    message_format_id(msg->payload.split.delim2, repr);
    break;
  };

  char_vector_append(repr, '\0');
  dbgln("%s", repr->data);
  char_vector_destroy(repr);
#endif
}