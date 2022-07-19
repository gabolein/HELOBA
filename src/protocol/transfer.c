#define LOG_LEVEL DEBUG_LEVEL
#define LOG_LABEL "Transfer"

#include "src/protocol/transfer.h"
#include "lib/logger.h"
#include "lib/random.h"
#include "lib/time_util.h"
#include "src/protocol/cache.h"
#include "src/protocol/message.h"
#include "src/protocol/message_formatter.h"
#include "src/protocol/message_util.h"
#include "src/protocol/tree.h"
#include "src/state.h"
#include "src/transport.h"

extern handler_f auto_handlers[MESSAGE_ACTION_COUNT][MESSAGE_TYPE_COUNT];

void register_automatic_transfer_handlers() {
  auto_handlers[DO][TRANSFER] = handle_do_transfer;
  auto_handlers[WILL][TRANSFER] = handle_will_transfer;
  auto_handlers[DO][SPLIT] = handle_do_split;
}

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

bool perform_split(split_direction direction) {
  club_key_vector_t *keys = club_hashmap_keys(gs.members);
  unsigned nkeys;
  if ((nkeys = club_key_vector_size(keys)) == 0) {
    return false;
  }

  qsort(keys->data, nkeys, sizeof(routing_id_t), &id_order);

  routing_id_t delim1 = club_key_vector_at(keys, nkeys / 4);
  routing_id_t delim2 = club_key_vector_at(keys, nkeys / 2);

  message_t split_msg = message_create(DO, SPLIT);
  split_msg.payload.split.direction = direction;
  split_msg.payload.split.delim1 = delim1;
  split_msg.payload.split.delim2 = delim2;
  routing_id_t receivers = routing_id_create(everyone, NULL);

  transport_send_message(&split_msg, receivers);

  for (size_t i = 0; i <= nkeys / 2; i++) {
    club_hashmap_remove(gs.members, club_key_vector_at(keys, i));
    gs.scores.current--;
  }

  club_key_vector_destroy(keys);

  return true;
}

bool handle_do_split(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == SPLIT);
  assert(!(gs.id.layer & leader));

  split_direction direction = msg->payload.split.direction;
  routing_id_t delim1 = msg->payload.split.delim1;
  routing_id_t delim2 = msg->payload.split.delim2;

  // Not splittig, update cache
  if (id_order(gs.id.MAC, delim2.MAC) > 0) {
    rc_key_vector_t *keys = cache_contents();
    for (unsigned i = 0; i < rc_key_vector_size(keys); i++) {
      routing_id_t current = rc_key_vector_at(keys, i);

      if (cache_get(current).f != gs.frequency ||
          id_order(current.MAC, delim2.MAC) >= 0)
        continue;

      if (direction == SPLIT_UP) {
        cache_insert(current, tree_node_parent(gs.frequency));
      } else {
        if (id_order(current.MAC, delim1.MAC) <= 0) {
          cache_insert(current, tree_node_lhs(gs.frequency));
        }
        if (id_order(current.MAC, delim2.MAC) <= 0) {
          cache_insert(current, tree_node_rhs(gs.frequency));
        }
      }
    }
    return false;
  }

  // Splitting, destination depends on direction
  frequency_t destination;
  if (direction == SPLIT_UP) {
    destination = tree_node_parent(gs.frequency);
  } else {
    destination = id_order(gs.id.MAC, delim1.MAC) <= 0
                      ? tree_node_lhs(gs.frequency)
                      : tree_node_rhs(gs.frequency);
  }

  transport_change_frequency(destination);
  return perform_registration(destination);
}

bool join_filter(message_t *msg) {
  return msg->header.sender_id.layer & leader &&
         ((message_action(msg) == DO || message_action(msg) == DONT) &&
          message_type(msg) == TRANSFER);
}

bool perform_unregistration(frequency_t to) {
  if (to == gs.frequency) {
    warnln("Trying to leave for current frequency.\n");
    return false;
  }

  // FIXME: score sollte auch Leader selbst beinhalten
  message_t unregister = message_create(WILL, TRANSFER);
  unregister.payload.transfer = (transfer_payload_t){.to = to};
  routing_id_t receiver = routing_id_create(everyone, NULL);
  transport_send_message(&unregister, receiver);
  gs.registered = false;

  if (gs.id.layer & leader) {
    gs.id.layer &= ~leader;
    gs.scores.current = 0;
    gs.scores.previous = 0;
    club_hashmap_clear(gs.members);
  }
  return true;
}

bool handle_join_answer(message_t *answer) {
  assert(message_type(answer) == TRANSFER);
  assert(message_action(answer) == DO || message_action(answer) == DONT);

  if (message_action(answer) == DONT) {
    dbgln("Leader rejected our join request.");
    return false;
  } else {
    dbgln("Leader accepted our join request.");
    gs.registered = true;
    gs.frequency = answer->payload.transfer.to;
    return true;
  }
}

bool perform_registration(frequency_t to) {
  // clang-format off
  // Leader election algorithm:
  // 1. listen for random time N \in [MIN, MAX]
  // 2. if someone else is sending on frequency, goto 5
  // 3. send WILL TRANSFER immediately
  // 4. receive all incoming messages
  // 4.1. if leader answered, go to 6
  // 4.2. if non-leader also sent WILL TRANSFER, goto 1
  // 4.3. else elect yourself as leader
  // 5. Timeout for time T or until leader message received (until we're sure election is finished)
  // 6. if not already registered, send WILL TRANSFER
  // clang-format on

  message_vector_t *received = message_vector_create();

  message_t join_request = message_create(WILL, TRANSFER);
  join_request.payload.transfer = (transfer_payload_t){.to = to};
  routing_id_t receiver = routing_id_create(leader, NULL);

  while (true) {
    size_t listen_ms = random_number_between(0, 50);
    if (transport_channel_active(listen_ms)) {
      break;
    }

    dbgln("Starting election");
    transport_send_message(&join_request, receiver);

    if (transport_channel_active(100)) {
      collect_messages(50, 1, join_filter, received);

      if (message_vector_size(received) > 0) {
        message_t answer = message_vector_at(received, 0);
        return handle_join_answer(&answer);
      }
    } else {
      dbgln("Nobody active on frequency, I am electing myself as leader.");
      gs.id.layer |= leader;
      gs.registered = true;
      gs.frequency = to;
      return true;
    }

    message_vector_clear(received);
  }

  dbgln("Lost Leader election race, sleeping until election is finished.");
  // TODO: in Config packen, testen wie lange Election maximal dauert
  sleep_ms(500);
  transport_send_message(&join_request, receiver);

  // NOTE: was wenn Leader nicht antwortet?
  collect_messages(50, 1, join_filter, received);
  if (message_vector_size(received) == 0) {
    warnln("Leader didn't answer join request.");
    return false;
  }

  message_t answer = message_vector_at(received, 0);
  return handle_join_answer(&answer);
}

bool handle_do_transfer(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == TRANSFER);

  if (msg->header.sender_id.layer != leader) {
    dbgln("Received DO TRANSFER from non-leader, ignoring.");
    return false;
  }

  frequency_t destination = msg->payload.transfer.to;
  transport_change_frequency(destination);
  return perform_registration(destination);
}

bool handle_register(routing_id_t registering) {
  if (!(gs.id.layer & leader)) {
    return true;
  }

  if (!club_hashmap_exists(gs.members, registering)) {
    club_hashmap_insert(gs.members, registering, true);
    gs.scores.current++;
    dbgln("Will Transfer: Node is registering. Membercount: %u",
          gs.scores.current);
  }

  // NOTE: Also send to registering nodes if they are already registered?
  message_t response = message_create(DO, TRANSFER);
  response.payload.transfer = (transfer_payload_t){.to = gs.frequency};
  transport_send_message(&response, registering);

  return true;
}

bool handle_unregister(routing_id_t unregistering) {
  if (gs.id.layer & leader) {
    if (!club_hashmap_exists(gs.members, unregistering)) {
      warnln("Not registered node %s is trying to unregister.",
             format_routing_id(unregistering));
      return false;
    }

    club_hashmap_remove(gs.members, unregistering);
    gs.scores.current--;
    dbgln("Node %s is unregistering. Membercount: %u",
          format_routing_id(unregistering), gs.scores.current);
    return true;
  }

  if (unregistering.layer & leader) {
    dbgln("Current Leader %s is leaving.", format_routing_id(unregistering));
    perform_registration(gs.frequency);
  }

  return true;
}

bool handle_will_transfer(message_t *msg) {
  assert(message_action(msg) == WILL);
  assert(message_type(msg) == TRANSFER);

  frequency_t f = msg->payload.transfer.to;
  routing_id_t transfering = msg->header.sender_id;

  if (f != gs.frequency && !handle_unregister(transfering)) {
    warnln("Could not unregister Node %s.", format_routing_id(transfering));
    return false;
  }

  if (f == gs.frequency && !handle_register(transfering)) {
    warnln("Could not register Node %s.", format_routing_id(transfering));
    return false;
  }

  if (cache_hit(transfering) || ((gs.id.layer & leader) && f != gs.frequency)) {
    cache_insert(transfering, f);
  }

  return true;
}
