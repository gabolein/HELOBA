#include "src/protocol/message_handler.h"
#include "lib/datastructures/generic/generic_vector.h"
#include "lib/time_util.h"
#include "src/protocol/message.h"
#include "src/protocol/routing.h"
#include "src/protocol/search.h"
#include "src/protocol/tree.h"
#include "src/state.h"
#include "src/transport.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

typedef bool (*handler_f)(message_t *msg);

bool handle_do_mute(message_t *msg);
bool handle_dont_mute(message_t *msg);
bool handle_do_update(message_t *msg);
bool handle_do_swap(message_t *msg);
bool handle_will_swap(message_t *msg);
bool handle_wont_swap(message_t *msg);
bool handle_do_transfer(message_t *msg);
bool handle_will_transfer(message_t *msg);
bool handle_do_find(message_t *msg);
bool handle_will_find(message_t *msg);
bool handle_do_migrate(message_t *msg);

static handler_f message_handlers[MESSAGE_ACTION_COUNT][MESSAGE_TYPE_COUNT] = {
    [DO][MUTE] = handle_do_mute,
    [DONT][MUTE] = handle_dont_mute,
    [DO][SWAP] = handle_do_swap,
    [WILL][SWAP] = handle_will_swap,
    [WONT][SWAP] = handle_wont_swap,
    [DO][TRANSFER] = handle_do_transfer,
    [WILL][TRANSFER] = handle_will_transfer,
    [DO][FIND] = handle_do_find,
    [WILL][FIND] = handle_will_find,
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

// TODO: Der mit dem man gerade SWAP macht, sollte irgendwo im Swap State
// gespeichert werden, dann muss man das nicht hier extra als Argument übergeben
void reject_swap(routing_id_t receiver) {
  message_t answer = message_create(WONT, SWAP);
  answer.payload.swap = (swap_payload_t){
      .with = gs.frequency,
      .score = gs.scores.current,
  };

  transport_send_message(&answer, receiver);
}

void accept_swap(routing_id_t receiver) {
  message_t answer = message_create(WILL, SWAP);
  answer.payload.swap = (swap_payload_t){
      .with = gs.frequency,
      .score = gs.scores.current,
  };

  transport_send_message(&answer, receiver);
}

void perform_swap(frequency_t to) {
  // NOTE: wie stellen wir sicher, dass das ohne Probleme auf beiden Frequenzen
  // zeitverschoben passieren kann?
  // Was im Moment passieren könnte:
  // 1. A schickt DO SWAP zu B
  // 2. A akzeptiert, sendet Antwort
  // 3. Alle Listeners auf A wechseln zu B
  // 4. B bekommt Antwort
  // 5. Alle Listeners auf B wechseln zu A
  // Nach diesem Austausch würden alle Listener auf Frequenz A sein und keiner
  // auf B, weil der Austausch nicht gleichzeitig passiert ist.
  // NOTE: die einfachste Lösung ist es, bei TRANSFER mitzusenden, ob er Teil
  // eines SWAPs ist. Wenn wir dann bei einem TRANSFER immer die alte Frequenz
  // abspeichern, können wir den zweiten falschen TRANSFER erkennen und
  // ignorieren.

  message_t migrate = message_create(DO, TRANSFER);
  migrate.payload.transfer = (transfer_payload_t){
      .to = to,
  };

  routing_id_t receivers = {.layer = everyone};
  transport_send_message(&migrate, receivers);
  transport_change_frequency(to);
}

// NOTE: braucht vllt Variable, in der steht, an welche Frequenz Swapping
// angefragt wurde
// FIXME: braucht Timeout, nach dem abgebrochen wird, weil es sein kann dass auf
// angefragter Frequenz niemand ist.
bool handle_do_swap(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == SWAP);

  if (!(gs.flags & LEADER) || msg->header.sender_id.layer != leader) {
    fprintf(stderr, "Only leaders should be able to swap, ignoring.\n");
    return false;
  }

  // FIXME: hier müssen irgendwo noch alle nonleader gemutet werden. Macht das
  // der Antragssteller, nachdem er auf Frequenz wechselt oder macht man das
  // selbst?
  if (gs.flags & TREE_SWAPPING) {
    fprintf(stderr, "Received DO SWAP while in the middle of another tree "
                    "operation, rejecting.\n");
    reject_swap(msg->header.sender_id);
    return false;
  }

  frequency_t f = msg->payload.swap.with;
  if (!is_valid_tree_node(f) || f == gs.frequency) {
    fprintf(stderr, "Sent frequency is invalid, ignoring.\n");
    reject_swap(msg->header.sender_id);
    return false;
  }

  // NOTE: eigener Activity Score muss immer noch irgendwo berechnet werden.
  uint8_t score = msg->payload.swap.score;
  if ((score <= gs.scores.current && tree_node_gt(gs.frequency, f)) ||
      (score >= gs.scores.current && tree_node_gt(f, gs.frequency))) {
    fprintf(stderr,
            "Tree Order is still preserved, I see no reason to swap!\n");
    reject_swap(msg->header.sender_id);
    return true;
  }

  accept_swap(msg->header.sender_id);
  perform_swap(f);

  return true;
}

bool handle_will_swap(message_t *msg) {
  assert(message_action(msg) == WILL);
  assert(message_type(msg) == SWAP);

  if (!(gs.flags & TREE_SWAPPING)) {
    fprintf(
        stderr,
        "Received SWAP confirmation, but didn't initiate myself, ignoring.\n");
    return false;
  }

  if (!(gs.flags & LEADER) || msg->header.sender_id.layer != leader) {
    fprintf(stderr, "Only leaders should be able to swap, ignoring.\n");
    return false;
  }

  perform_swap(msg->payload.swap.with);
  gs.flags &= ~TREE_SWAPPING;
  return true;
}

bool handle_wont_swap(message_t *msg) {
  assert(message_action(msg) == WONT);
  assert(message_type(msg) == SWAP);

  gs.flags &= ~TREE_SWAPPING;
  return true;
}

bool handle_do_migrate(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == MIGRATE);

  if (msg->header.sender_id.layer != leader) {
    fprintf(stderr, "Received DO TRANSFER from non-leader, ignoring.\n");
    return false;
  }

  frequency_t destination = msg->payload.transfer.to;

  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC_RAW, &current);
  struct timespec migration_blocked =
      timestamp_add_ms(gs.migrate.last_migrate, 10);
  if (destination == gs.migrate.old &&
      timestamp_cmp(migration_blocked, current) == 1)
    return false;

  return transport_change_frequency(destination);
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

  message_t join_request = message_create(WILL, TRANSFER);
  join_request.payload.transfer = (transfer_payload_t){.to = destination};

  routing_id_t receiver = {.layer = leader};
  transport_send_message(&join_request, receiver);

  return true;
}

bool handle_will_transfer(message_t *msg) {
  assert(message_action(msg) == WILL);
  assert(message_type(msg) == TRANSFER);

  if (gs.id.layer == leader) {
    frequency_t f = msg->payload.transfer.to;

    if (f == gs.frequency) {
      if (!club_hashmap_exists(gs.members, f)) {
        return false;
      }

      club_hashmap_remove(gs.members, f);
      gs.scores.current--;
    } else {
      club_hashmap_insert(gs.members, f, true);
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
  transport_get_id(self_id.optional_MAC);
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

MAKE_SPECIFIC_VECTOR_SOURCE(message_t, message)

// FIXME: das ist ein extrem schlechter Name, pls fix
void message_assign_collector(unsigned timeout_ms, unsigned max_messages,
                              filter_f filter, message_vector_t *collector) {
  message_t msg;
  struct timespec start_time;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
  while (!hit_timeout(timeout_ms, &start_time)) {
    if (transport_receive_message(&msg) && filter(&msg)) {
      message_vector_append(collector, msg);
      if (message_vector_size(collector) == max_messages) {
        return;
      }
    }
  }
}
