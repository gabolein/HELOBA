#define LOG_LEVEL DEBUG_LEVEL
#define LOG_LABEL "Transfer"

#include "src/protocol/transfer.h"
#include "lib/logger.h"
#include "lib/random.h"
#include "lib/time_util.h"
#include "src/protocol/message.h"
#include "src/protocol/message_util.h"
#include "src/protocol/routing.h"
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
  routing_id_t_vector_t *keys = club_hashmap_keys(gs.members);
  unsigned nkeys;
  if ((nkeys = routing_id_t_vector_size(keys)) == 0) {
    return false;
  }

  qsort(keys->data, nkeys, sizeof(routing_id_t), &id_order);

  // NOTE nkeys/x -1??
  routing_id_t delim1 = routing_id_t_vector_at(keys, nkeys / 4);
  routing_id_t delim2 = routing_id_t_vector_at(keys, nkeys / 2);

  message_t split_msg = message_create(DO, SPLIT);
  split_msg.payload.split.delim1 = delim1;
  split_msg.payload.split.delim2 = delim2;
  routing_id_t receivers = routing_id_create(everyone, NULL);

  transport_send_message(&split_msg, receivers);

  for (size_t i = 0; i <= nkeys / 2; i++) {
    club_hashmap_remove(gs.members, routing_id_t_vector_at(keys, i));
    gs.scores.current--;
  }

  routing_id_t_vector_destroy(keys);

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
    // TODO node that does not split update cache
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

bool election_filter(message_t *msg) {
  return msg->header.sender_id.layer & leader ||
         (message_action(msg) == WILL && message_type(msg) == TRANSFER);
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

  bool participating = true;
  bool leader_detected = false;
  message_vector_t *received = message_vector_create();

  message_t join_request = message_create(WILL, TRANSFER);
  join_request.payload.transfer =
      (transfer_payload_t){.to = gs.frequencies.current};
  routing_id_t receiver = routing_id_create(leader, NULL);

  while (participating) {
    size_t listen_ms = random_number_between(0, 50);
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
    while (!hit_timeout(listen_ms, &start_time) && participating) {
      if (transport_channel_active()) {
        participating = false;
      }
    }

    if (participating) {
      dbgln("Registration: starting election");
      transport_send_message(&join_request, receiver);
      // NOTE: selbst mit einem Timeout von 5000ms kommen hier keine Nachrichten
      // an, obwohl der andere Node auf jeden Fall sendet, irgendetwas
      // funktioniert in dieser Funktion noch nicht.
      collect_messages(100, UINT_MAX, election_filter, received);

      if (message_vector_size(received) == 0) {
        dbgln("Nobody active on frequency, I am electing myself as leader.");
        gs.id.layer |= leader;
        participating = false;
      }

      for (unsigned i = 0; i < message_vector_size(received) && participating;
           i++) {
        message_t current = message_vector_at(received, i);
        if (current.header.sender_id.layer & leader) {
          leader_detected = true;
          participating = false;

          if (message_action(&current) == DONT) {
            dbgln("Leader rejected our join request.");
            return false;
          } else {
            dbgln("Leader accepted our join request.");
            gs.registered = true;
            return true;
          }
        }
      }

      message_vector_clear(received);
    }
  }

  if (gs.id.layer & leader) {
    gs.registered = true;
    dbgln("Registration result: became leader");
    return true;
  }

  if (!leader_detected) {
    // NOTE: wir landen hier auch im Normalfall, wenn es schon einen Leader auf
    // der Frequenz gibt. Wir sollten dann irgendwie einen Weg finden, nicht in
    // diesem Timeout zu landen.
    dbgln("Registration: No Leader detected, sleeping for a bit.");
    sleep_ms(500);
  }

  dbgln("Registration: sending WILL TRANSFER");
  transport_send_message(&join_request, receiver);
  // NOTE: ist es sicher, dass der received vector hier wieder leer ist?
  collect_messages(50, 1, join_filter, received);

  if (message_vector_size(received) == 0) {
    warnln("Leader didn't answer join request.");
    return false;
  }

  message_t answer = message_vector_at(received, 0);
  if (message_action(&answer) == DONT) {
    dbgln("Leader rejected our join request.");
    return false;
  } else {
    dbgln("Leader accepted our join request.");
    gs.registered = true;
    return true;
  }
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

  if (gs.id.layer & leader) {
    frequency_t f = msg->payload.transfer.to;
    routing_id_t nonleader = msg->header.sender_id;

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

  // TODO: Cache Handling
  return true;
}
