#define LOG_LEVEL DEBUG_LEVEL
#define LOG_LABEL "Transfer"

#include "src/protocol/transfer.h"
#include "lib/logger.h"
#include "lib/random.h"
#include "lib/time_util.h"
#include "src/protocol/cache.h"
#include "src/protocol/message.h"
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

bool perform_split() {
  club_key_vector_t *keys = club_hashmap_keys(gs.members);
  unsigned nkeys;
  if ((nkeys = club_key_vector_size(keys)) == 0) {
    return false;
  }

  qsort(keys->data, nkeys, sizeof(routing_id_t), &id_order);

  // NOTE nkeys/x -1??
  routing_id_t delim1 = club_key_vector_at(keys, nkeys / 4);
  routing_id_t delim2 = club_key_vector_at(keys, nkeys / 2);

  message_t split_msg = message_create(DO, SPLIT);
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

  bool split = true;
  frequency_t destination;
  routing_id_t delim1 = msg->payload.split.delim1;
  routing_id_t delim2 = msg->payload.split.delim2;

  if (id_order(gs.id.MAC, delim1.MAC) <= 0) {
    destination = tree_node_lhs(gs.frequencies.current);
  } else if (id_order(gs.id.MAC, delim2.MAC) <= 0) {
    destination = tree_node_rhs(gs.frequencies.current);
    transport_change_frequency(tree_node_rhs(gs.frequencies.current));
  } else {
    split = false;
    rc_key_vector_t *keys = cache_contents();

    for (unsigned i = 0; i < rc_key_vector_size(keys); i++) {
      routing_id_t current = rc_key_vector_at(keys, i);

      if (id_order(gs.id.MAC, delim1.MAC) <= 0) {
        cache_insert(current, tree_node_lhs(gs.frequencies.current));
      }

      if (id_order(gs.id.MAC, delim2.MAC) <= 0) {
        cache_insert(current, tree_node_rhs(gs.frequencies.current));
      }
    }
  }

  if (split) {
    transport_change_frequency(destination);
    perform_registration();
    message_t join_request = message_create(WILL, TRANSFER);
    join_request.payload.transfer = (transfer_payload_t){.to = destination};
    routing_id_t receiver = routing_id_create(leader, NULL);
    transport_send_message(&join_request, receiver);
  }

  return split;
}

bool join_filter(message_t *msg) {
  return msg->header.sender_id.layer & leader &&
         ((message_action(msg) == DO || message_action(msg) == DONT) &&
          message_type(msg) == TRANSFER);
}

bool perform_unregistration(frequency_t to) {
  if (to == gs.frequencies.current) {
    warnln("Trying to leave for current frequency.\n");
    return false;
  }

  // FIXME: score sollte auch Leader selbst beinhalten
  if (gs.id.layer & leader) {
    if (gs.scores.current > 0) {
      // TODO: anderen Node auf Frequenz explizit zum Leader machen und ihm
      // aktuellen Status mitschicken.
    }

    gs.id.layer &= ~leader;
    gs.scores.current = 0;
    gs.scores.previous = 0;
    club_hashmap_clear(gs.members);
  } else {
    message_t unregister = message_create(WILL, TRANSFER);
    unregister.payload.transfer = (transfer_payload_t){.to = to};
    routing_id_t receiver = routing_id_create(leader, NULL);
    transport_send_message(&unregister, receiver);
  }

  gs.registered = false;
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
    return true;
  }
}

bool perform_registration() {
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
  join_request.payload.transfer =
      (transfer_payload_t){.to = gs.frequencies.current};
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
      return true;
    }

    message_vector_clear(received);
  }

  dbgln("Lost Leader election race, sleeping until election is finished.");
  // TODO: in Config packen, testen wie lange Election maximal dauert
  sleep_ms(500);
  transport_send_message(&join_request, receiver);

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
  return perform_registration();
}

bool handle_will_transfer(message_t *msg) {
  assert(message_action(msg) == WILL);
  assert(message_type(msg) == TRANSFER);

  frequency_t f = msg->payload.transfer.to;
  routing_id_t nonleader = msg->header.sender_id;

  if (gs.id.layer & leader) {

    if (f != gs.frequencies.current) {
      if (!club_hashmap_exists(gs.members, nonleader)) {
        warnln("Will Transfer:"
               "not registered node is trying to unregister.");
        return false;
      }

      club_hashmap_remove(gs.members, nonleader);
      gs.scores.current--;
      dbgln("Will Transfer: Node is unregistering."
            " Membercount: %u",
            gs.scores.current);
    } else {
      if (!club_hashmap_exists(gs.members, nonleader)) {
        club_hashmap_insert(gs.members, nonleader, true);
        gs.scores.current++;
        dbgln("Will Transfer: Node is registering."
              " Membercount: %u",
              gs.scores.current);
      }
    }

    // NOTE also send to registering nodes if they are already registered?
    message_t response = message_create(DO, TRANSFER);
    response.payload.transfer = (transfer_payload_t){.to = f};
    transport_send_message(&response, msg->header.sender_id);
  }

  if ((gs.id.layer & leader) && f != gs.frequencies.current) {
    cache_insert(nonleader, f);
    return true;
  }

  if (cache_hit(nonleader)) {
    if (f != gs.frequencies.current) {
      cache_insert(nonleader, f);
    } else {
      cache_remove(nonleader);
    }
  }

  return true;
}
