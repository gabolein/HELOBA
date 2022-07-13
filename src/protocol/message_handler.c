#include "src/protocol/message_handler.h"
#include "src/protocol/message.h"
#include "src/protocol/routing.h"
#include "src/protocol/search.h"
#include "src/protocol/tree.h"
#include "src/state.h"
#include "src/transport.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

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

static handler_f message_handlers[MESSAGE_ACTION_COUNT][MESSAGE_TYPE_COUNT] = {
    [DO][MUTE] = handle_do_mute,
    [DONT][MUTE] = handle_dont_mute,
    [DO][SWAP] = handle_do_swap,
    [WILL][SWAP] = handle_will_swap,
    [WONT][SWAP] = handle_wont_swap,
    [DO][TRANSFER] = handle_do_transfer,
    [WILL][TRANSFER] = handle_will_transfer,
    [DO][FIND] = handle_do_find,
    [WILL][FIND] = handle_will_find};

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

void reject_do_swap() {
  message_t answer;
  answer.header.action = WONT;
  answer.header.type = SWAP;
  // FIXME: Die IDs müssen genauer gesetzt werden, im VIRTUAL Modus
  // kann es sonst sein, dass wir unsere eigene Nachricht bekommen.
  answer.header.receiver_id.layer = leader;
  answer.header.sender_id = gs.id;
  answer.payload.swap.with = gs.frequency;
  answer.payload.swap.score = gs.scores.current;

  transport_send_message(&answer);
}

void accept_do_swap() {
  message_t answer;
  answer.header.action = WILL;
  answer.header.type = SWAP;
  // FIXME: Die IDs müssen genauer gesetzt werden, im VIRTUAL Modus
  // kann es sonst sein, dass wir unsere eigene Nachricht bekommen.
  answer.header.receiver_id.layer = leader;
  answer.header.sender_id = gs.id;
  answer.payload.swap.with = gs.frequency;
  answer.payload.swap.score = gs.scores.current;

  transport_send_message(&answer);
}

// FIXME: braucht vllt Variable, in der steht, an welche Frequenz Swapping
// angefragt wurde
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
    reject_do_swap();
    // NOTE: muss hier false oder true zurückgegeben werden?
    return true;
  }

  frequency_t f = msg->payload.swap.with;
  if (!is_valid_tree_node(f) || f == gs.frequency) {
    fprintf(stderr, "Sent frequency is invalid, ignoring.\n");
    // NOTE: sollte hier noch reject_do_swap() aufgerufen werden?
    return false;
  }

  // NOTE: eigener Activity Score muss immer noch irgendwo berechnet werden.
  uint8_t score = msg->payload.swap.score;
  if ((score <= gs.scores.current && tree_node_gt(gs.frequency, f)) ||
      (score >= gs.scores.current && tree_node_gt(f, gs.frequency))) {
    fprintf(stderr,
            "Tree Order is still preserved, I see no reason to swap!\n");
    reject_do_swap();
    return true;
  }

  accept_do_swap();

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
  message_t migrate;
  migrate.header.action = DO;
  migrate.header.type = TRANSFER;
  migrate.header.receiver_id.layer = everyone;
  migrate.header.sender_id = gs.id;
  migrate.payload.transfer.to = f;

  transport_send_message(&migrate);
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

  frequency_t f = msg->payload.swap.with;

  message_t migrate;
  migrate.header.action = DO;
  migrate.header.type = TRANSFER;
  migrate.header.receiver_id.layer = everyone;
  migrate.header.sender_id = gs.id;
  migrate.payload.transfer.to = f;

  transport_send_message(&migrate);

  gs.flags &= ~TREE_SWAPPING;
  return true;
}

bool handle_wont_swap(message_t *msg) {
  assert(message_action(msg) == WONT);
  assert(message_type(msg) == SWAP);

  gs.flags &= ~TREE_SWAPPING;
  return true;
}

bool handle_do_transfer(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == TRANSFER);

  if (msg->header.sender_id.layer != leader) {
    fprintf(stderr, "Received DO TRANSFER from non-leader, ignoring.\n");
    return false;
  }

  frequency_t destination_frequency = msg->payload.transfer.to;
  transport_change_frequency(destination_frequency);
  message_t join_request;
  join_request.header.action = WILL;
  join_request.header.type = TRANSFER;
  join_request.header.sender_id.layer = specific;
  transport_get_id(join_request.header.sender_id.optional_MAC);
  join_request.header.receiver_id.layer = leader;
  join_request.payload.transfer.to = destination_frequency;

  transport_send_message(&join_request);

  return true;
}

bool handle_will_transfer(message_t *msg) {
  assert(message_action(msg) == WILL);
  assert(message_type(msg) == TRANSFER);

  // TODO: my_id muss irgendwo noch initialisiert werden
  if (gs.id.layer == leader) {
    frequency_t f = msg->payload.transfer.to;

    if (f == gs.frequency) {
      if (!club_hashmap_exists(gs.members, f)) {
        return false;
      }

      club_hashmap_delete(gs.members, f);
    } else {
      club_hashmap_insert(gs.members, f, true);
    }
  }

  // TODO: Cache Handling

  return true;
}

bool set_sender_id_layer(routing_id_t *sender_id) {
  if (gs.flags & LEADER) {
    sender_id->layer |= leader;
  } else {
    sender_id->layer &= ~leader;
  }

  return true;
}

message_header_t will_find_assemble_header(routing_id_t sender_id,
                                           routing_id_t receiver_id) {
  return (message_header_t){WILL, FIND, sender_id, receiver_id};
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

  message_t reply_msg;
  set_sender_id_layer(&self_id);
  reply_msg.header = will_find_assemble_header(self_id, msg->header.sender_id);

  transport_send_message(&reply_msg);

  return true;
}

bool handle_will_find(message_t *msg) {
  assert(message_action(msg) == WILL);
  assert(message_type(msg) == FIND);

  if (!(gs.flags & SEARCHING)) {
    fprintf(stderr, "Got WILL FIND, but didn't start search.\n");
    return false;
  }

  routing_id_t sender = msg->header.sender_id;

  if (routing_id_MAC_equal(sender, get_to_find())) {
    search_concluded();
    gs.search.found = true;
  } else {
    // FIXME: sollte vielleicht direkt im Paket gesendet werden.
    search_hint_t hint = {
        .type = CACHE,
        .f = msg->payload.find.cached,
    };

    search_queue_add(hint);
  }

  return true;
}

bool handle_message(message_t *msg) {
  assert(message_is_valid(msg));

  return message_handlers[message_action(msg)][message_type(msg)](msg);
}
