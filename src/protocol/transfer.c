#include "src/protocol/tree.h"
#include "src/state.h"
#include "src/transport.h"

int id_order(const void *arg1, const void *arg2) {
  uint8_t *id_1 = (uint8_t *)arg1;
  uint8_t *id_2 = (uint8_t *)arg2;

  for (size_t i = 0; i < MAC_SIZE; i++) {
    if (id_1[i] > id_2[i]) {
      return 1;
    }

    if (id_1[i] < id_2[i]) {
      return -1;
    }
  }

  return 0;
}

bool perform_split() {
  routing_id_t_vector_t *keys = club_hashmap_keys(gs.members);
  unsigned nkeys = routing_id_t_vector_size(keys);
  qsort(keys, sizeof(routing_id_t), nkeys, &id_order);

  // NOTE nkeys/x -1??
  routing_id_t delim1 = routing_id_t_vector_at(keys, nkeys / 4);
  routing_id_t delim2 = routing_id_t_vector_at(keys, nkeys / 2);

  message_t split_msg = message_create(DO, SPLIT);
  split_msg.payload.split.delim1 = delim1;
  split_msg.payload.split.delim2 = delim2;
  routing_id_t receivers = {.layer = everyone};

  transport_send_message(&split_msg, receivers);

  for (size_t i = 0; i < nkeys / 2; i++) {
    club_hashmap_remove(gs.members, routing_id_t_vector_at(keys, i));
    gs.scores.current--;
  }

  routing_id_t_vector_destroy(keys);

  return true;
}

bool handle_do_split(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == MUTE);
  assert(!(gs.flags & LEADER));

  bool split = true;
  frequency_t to;
  routing_id_t delim1 = msg->payload.split.delim1;
  routing_id_t delim2 = msg->payload.split.delim2;

  if (id_order(gs.id.MAC, delim1.MAC) <= 0) {
    to = tree_node_lhs(gs.frequencies.current);
  } else if (id_order(gs.id.MAC, delim2.MAC) <= 0) {
    to = tree_node_rhs(gs.frequencies.current);
    transport_change_frequency(to);
  } else {
    split = false;
    // TODO node that does not swap update cache
  }

  if (split) {
    transport_change_frequency(to);
    // TODO election
    message_t join_request = message_create(WILL, TRANSFER);
    join_request.payload.transfer = (transfer_payload_t){.to = to};
    routing_id_t receiver = {.layer = leader};
    transport_send_message(&join_request, receiver);
  }

  return split;
}
