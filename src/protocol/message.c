#define LOG_LEVEL WARNING_LEVEL
#define LOG_LABEL "Message"

#include "src/protocol/message.h"
#include "lib/datastructures/generic/generic_priority_queue.h"
#include "lib/datastructures/generic/generic_vector.h"
#include "lib/logger.h"
#include "src/state.h"
#include "src/transport.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

MAKE_SPECIFIC_VECTOR_SOURCE(message_t, message)

static bool message_allowlist[MESSAGE_ACTION_COUNT][MESSAGE_TYPE_COUNT] = {
    [DO][SWAP] = true,     [WILL][SWAP] = true,     [WONT][SWAP] = true,
    [DO][TRANSFER] = true, [WILL][TRANSFER] = true, [DO][FIND] = true,
    [WILL][FIND] = true,   [WILL][HINT] = true,     [DO][MIGRATE] = true,
    [DO][SPLIT] = true};

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

  if (msg->header.receiver_id.layer & specific) {
    return routing_id_equal(msg->header.receiver_id, id);
  }

  if ((msg->header.receiver_id.layer & leader) == (id.layer & leader)) {
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

routing_id_t routing_id_create(routing_layer_t layer, uint8_t *MAC) {
  if (layer & specific) {
    assert(MAC != NULL);
  }

  routing_id_t id;
  memset(&id, 0, sizeof(id));
  id.layer = layer;
  if (layer & specific) {
    memcpy(id.MAC, MAC, MAC_SIZE);
  }

  return id;
}
