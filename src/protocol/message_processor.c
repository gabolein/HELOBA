#include "lib/datastructures/generic/generic_priority_queue.h"
#include "src/protocol/message.h"
#include "src/transport.h"

/*int message_cmp(message_t *a, message_t *b);*/

/*MAKE_SPECIFIC_PRIORITY_QUEUE_HEADER(message_t *, message)*/
/*MAKE_SPECIFIC_PRIORITY_QUEUE_SOURCE(message_t *, message, message_cmp)*/

/*static message_priority_queue_t *messages;*/

#define CHANNEL_CONTROL_PRIORITY 3
#define TREE_OPERATION_PRIORITY 2
#define BOOKKEEPING_PRIORITY 1
#define REQUEST_PRIORITY 0

static uint8_t type_priorities[MESSAGE_TYPE_COUNT] = {
    [MUTE] = CHANNEL_CONTROL_PRIORITY, [UPDATE] = TREE_OPERATION_PRIORITY,
    [SWAP] = TREE_OPERATION_PRIORITY,  [REPORT] = BOOKKEEPING_PRIORITY,
    [TRANSFER] = BOOKKEEPING_PRIORITY, [FIND] = REQUEST_PRIORITY};

static bool message_allowlist[MESSAGE_ACTION_COUNT][MESSAGE_TYPE_COUNT] = {
    [DO][MUTE] = true,
    [DONT][MUTE] = true,
    [DO][UPDATE] = true,
    [DO][SWAP] = true,
    [WILL][SWAP] = true,
    [WONT][SWAP] = true,
    [DO][REPORT] = true,
    //  NOTE: WILL ist hier ziemlich komisch, dieser Typ bedeutet nur die
    //  Antwort auf ein DO, entweder man findet einen Weg den Typ besser zu
    //  benennen, oder wir fügen noch ein ACK hinzu, um solche Fälle
    //  abzudecken.
    [WILL][REPORT] = true,
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
  if (msg->header.sender_id.layer != specific) {
    fprintf(stderr, "[WARNING] Message sender field is not set.\n");
    return false;
  }

  return true;
}

bool message_is_for_node(message_t *msg) {
  message_header_t header = msg->header;
  routing_id_t self_id;
  transport_get_id(self_id.optional_MAC);

  // Unless registered, we only expect messages from the leader specifically to
  // us
  // TODO get registered information from somewhere else
  if (false) {
    /*if(!(global_flags & REGISTERED)){ */
    return header.sender_id.layer == leader &&
           header.receiver_id.layer == specific &&
           routing_id_MAC_equal(self_id, header.receiver_id);
  }

  return header.receiver_id.layer == everyone ||
         routing_id_MAC_equal(self_id, header.receiver_id);
}

bool process_message(message_t *msg) {
  return message_is_valid(msg) && message_is_for_node(msg);
}

// TODO: Score Formel schreiben, die Wichtigkeit von Nachrichten im Netzwerk
// richtig darstellt. Das hier ist nur ein erster grober Versuch.
uint8_t message_priority_score(message_t *msg) {
  uint8_t score = type_priorities[message_type(msg)];

  if (message_is_command(msg))
    score *= 2;

  if (message_from_leader(msg))
    score += 6;

  return score;
}

int message_cmp(message_t *a, message_t *b) {
  uint8_t score_a = message_priority_score(a);
  uint8_t score_b = message_priority_score(b);

  if (score_a > score_b)
    return 1;
  if (score_a < score_b)
    return -1;

  return 0;
}

/*void message_processor_initialize() {*/
/*messages = message_priority_queue_create();*/
/*}*/

/*bool has_pending_received_messages() {*/
/*return message_priority_queue_size(messages) > 0;*/
/*}*/

/*message_t *next_pending_received_message() {*/
/*return message_priority_queue_pop(messages);*/
/*}*/
