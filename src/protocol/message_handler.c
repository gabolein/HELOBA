#include "src/protocol/message_handler.h"
#include "lib/datastructures/generic/generic_vector.h"
#include "lib/random.h"
#include "lib/time_util.h"
#include "src/protocol/message.h"
#include "src/protocol/message_util.h"
#include "src/protocol/routing.h"
#include "src/protocol/search.h"
#include "src/protocol/swap.h"
#include "src/protocol/tree.h"
#include "src/state.h"
#include "src/transport.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

typedef bool (*handler_f)(message_t *msg);

bool handle_do_mute(message_t *msg);
bool handle_dont_mute(message_t *msg);
bool handle_do_update(message_t *msg);
bool handle_do_swap(message_t *msg);
bool handle_do_transfer(message_t *msg);
bool handle_will_transfer(message_t *msg);
bool handle_do_find(message_t *msg);
bool handle_do_migrate(message_t *msg);

static handler_f message_handlers[MESSAGE_ACTION_COUNT][MESSAGE_TYPE_COUNT] = {
    [DO][MUTE] = handle_do_mute,
    [DONT][MUTE] = handle_dont_mute,
    [DO][SWAP] = handle_do_swap,
    [DO][TRANSFER] = handle_do_transfer,
    [WILL][TRANSFER] = handle_will_transfer,
    [DO][FIND] = handle_do_find,
    [DO][MIGRATE] = handle_do_migrate};

bool handle_do_mute(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == MUTE);

  if (!message_from_leader(msg)) {
    fprintf(stderr, "Received DO MUTE from non-leader, will not act on it.\n");
    return false;
  }

  gs.flags |= MUTED;

  return true;
}

bool handle_dont_mute(message_t *msg) {
  assert(message_action(msg) == DONT);
  assert(message_type(msg) == MUTE);

  if (!message_from_leader(msg)) {
    fprintf(stderr,
            "Received DONT MUTE from non-leader, will not act on it.\n");
    return false;
  }

  gs.flags &= ~MUTED;

  return true;
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

bool handle_do_transfer(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == TRANSFER);

  if (msg->header.sender_id.layer != leader) {
    fprintf(stderr, "Received DO TRANSFER from non-leader, ignoring.\n");
    return false;
  }

  frequency_t destination = msg->payload.transfer.to;
  transport_change_frequency(destination);

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
  join_request.payload.transfer = (transfer_payload_t){.to = destination};
  routing_id_t receiver = {.layer = leader};

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
      transport_send_message(&join_request, receiver);
      collect_messages(50, UINT_MAX, election_filter, received);

      if (message_vector_size(received) == 0) {
        gs.flags |= LEADER;
        gs.id.layer |= leader;
        participating = false;
      }

      for (unsigned i = 0; i < message_vector_size(received) && participating;
           i++) {
        message_t current = message_vector_at(received, i);
        if (current.header.sender_id.layer & leader) {
          leader_detected = true;
          participating = false;
        }
      }

      message_vector_clear(received);
    }
  }

  if (gs.flags & LEADER) {
    return true;
  }

  if (!leader_detected) {
    // NOTE: wir landen hier auch im Normalfall, wenn es schon einen Leader auf
    // der Frequenz gibt. Wir sollten dann irgendwie einen Weg finden, nicht in
    // diesem Timeout zu landen.
    sleep_ms(500);
  }

  transport_send_message(&join_request, receiver);
  // NOTE: ist es sicher, dass der received vector hier wieder leer ist?
  collect_messages(50, 1, join_filter, received);

  if (message_vector_size(received) == 0) {
    fprintf(stderr, "Leader didn't answer join request.\n");
    return false;
  }

  message_t answer = message_vector_at(received, 0);
  if (message_action(&answer) == DONT) {
    fprintf(stderr, "Leader rejected our join request.\n");
    return false;
  } else {
    fprintf(stderr, "Leader accepted our join request.\n");
    return true;
  }
}

bool handle_will_transfer(message_t *msg) {
  assert(message_action(msg) == WILL);
  assert(message_type(msg) == TRANSFER);

  if (gs.id.layer == leader) {
    frequency_t f = msg->payload.transfer.to;
    routing_id_t nonleader = msg->header.sender_id;

    if (f == gs.frequencies.current) {
      if (!club_hashmap_exists(gs.members, nonleader)) {
        return false;
      }

      club_hashmap_remove(gs.members, nonleader);
      gs.scores.current--;
    } else {
      club_hashmap_insert(gs.members, nonleader, true);
      gs.scores.current++;
    }
  }

  // TODO: Cache Handling

  return true;
}

bool handle_do_find(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == FIND);

  if (!(gs.flags & REGISTERED)) {
    return false;
  }

  routing_id_t to_find = msg->payload.find.to_find;
  routing_id_t self_id;
  transport_get_id(self_id.MAC);
  bool searching_for_self = routing_id_MAC_equal(to_find, self_id);

  if (!leader && !searching_for_self) {
    return false;
  }

  message_t reply = message_create(WILL, FIND);
  transport_send_message(&reply, msg->header.sender_id);

  return true;
}

bool handle_message(message_t *msg) {
  assert(message_is_valid(msg));

  return message_handlers[message_action(msg)][message_type(msg)](msg);
}
