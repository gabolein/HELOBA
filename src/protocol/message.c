#include "src/protocol/message.h"
#include "lib/datastructures/generic/generic_priority_queue.h"
#include "src/protocol/routing.h"
#include "src/state.h"
#include "src/transport.h"

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
    fprintf(stderr, "[WARNING] Message is NULL.\n");
    return false;
  }

  if (message_allowlist[message_action(msg)][message_type(msg)] == false) {
    fprintf(stderr, "[WARNING] Message has invalid ACTION/TYPE combination.\n");
    return false;
  }

  // FIXME: Nachrichten vom Leader würden im Moment als nicht valid angesehen
  // werden. Es sollte am besten so gemacht werden, dass leader ein weiteres Bit
  // ist, welches man separat von specific setzen kann.
  if (msg->header.sender_id.layer & specific) {
    fprintf(stderr, "[WARNING] Message sender field is not set.\n");
    return false;
  }

  return true;
}

bool routing_id_equal(routing_id_t id1, routing_id_t id2) {
  return (id1.layer & specific) && (id2.layer & specific) &&
         memcmp(id1.optional_MAC, id2.optional_MAC, MAC_SIZE) == 0;
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