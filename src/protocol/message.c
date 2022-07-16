#define LOG_LEVEL DEBUG_LEVEL
#define LOG_LABEL "Message"

#include "src/protocol/message.h"
#include "lib/datastructures/generic/generic_priority_queue.h"
#include "lib/datastructures/generic/generic_vector.h"
#include "lib/logger.h"
#include "src/protocol/routing.h"
#include "src/state.h"
#include "src/transport.h"

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
  msg.header.action = action;
  msg.header.type = type;

  return msg;
}

MAKE_SPECIFIC_VECTOR_HEADER(char, char) // Binks!
MAKE_SPECIFIC_VECTOR_SOURCE(char, char)

void repr_print_string(char *str, char_vector_t *repr) {
  for (unsigned i = 0; i < strlen(str); i++) {
    char_vector_append(repr, str[i]);
  }
}

void message_print_action(message_action_t action, char_vector_t *repr) {
  repr_print_string("Action: ", repr);

  switch (action) {
  case DO:
    repr_print_string("DO", repr);
  case DONT:
    repr_print_string("DONT", repr);
  case WILL:
    repr_print_string("WILL", repr);
  case WONT:
    repr_print_string("WONT", repr);
  }
}

void message_print_type(message_type_t type, char_vector_t *repr) {
  repr_print_string("Type: ", repr);

  switch (type) {
  case FIND:
    repr_print_string("FIND", repr);
  case SWAP:
    repr_print_string("SWAP", repr);
  case TRANSFER:
    repr_print_string("TRANSFER", repr);
  case MUTE:
    repr_print_string("MUTE", repr);
  case MIGRATE:
    repr_print_string("MIGRATE", repr);
  case SPLIT:
    repr_print_string("SPLIT", repr);
  }
}

void message_print_id(routing_id_t id, char_vector_t *repr) {
  repr_print_string("ID: ", repr);

  for (unsigned i = 0; i < 3; i++) {
    if (id.layer & (1 << i)) {
      switch (1 << i) {
      case leader:
        repr_print_string("Leader", repr);
      case everyone:
        repr_print_string("Everyone", repr);
      case specific:
        repr_print_string("Specific", repr);
      }
    }
  }
}

void message_print(message_t *msg) {
  char_vector_t *repr = char_vector_create();
}