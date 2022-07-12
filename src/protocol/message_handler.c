#include "src/protocol/message_handler.h"
#include "src/protocol/message.h"
#include "src/protocol/routing.h"
#include "src/protocol/search.h"
#include "src/transport.h"
#include "src/state.h"
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
bool handle_do_report(message_t *msg);
bool handle_will_report(message_t *msg);
bool handle_do_transfer(message_t *msg);
bool handle_will_transfer(message_t *msg);
bool handle_do_find(message_t *msg);
bool handle_will_find(message_t *msg);

void update_single_tree_node(local_tree_t *t, frequency_t old, frequency_t new);

static handler_f message_handlers[MESSAGE_ACTION_COUNT][MESSAGE_TYPE_COUNT] = {
    [DO][MUTE] = handle_do_mute,
    [DONT][MUTE] = handle_dont_mute,
    [DO][UPDATE] = handle_do_update,
    [DO][SWAP] = handle_do_swap,
    [WILL][SWAP] = handle_will_swap,
    [WONT][SWAP] = handle_wont_swap,
    [DO][REPORT] = handle_do_report,
    [WILL][REPORT] = handle_will_report,
    [DO][TRANSFER] = handle_do_transfer,
    [WILL][TRANSFER] = handle_will_transfer,
    [DO][FIND] = handle_do_find,
    [WILL][FIND] = handle_will_find};

int message_cmp(message_t a, message_t b);

MAKE_SPECIFIC_PRIORITY_QUEUE_HEADER(message_t, message)
MAKE_SPECIFIC_VECTOR_SOURCE(message_t, message);
MAKE_SPECIFIC_PRIORITY_QUEUE_SOURCE(message_t, message, message_cmp)

static message_priority_queue_t *to_send;

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

bool handle_do_update(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == UPDATE);

  update_single_tree_node(&gs.tree, msg->payload.update.old,
                          msg->payload.update.updated);

  return true;
}

frequency_t tree_select(local_tree_t *t, uint8_t m) {
  switch (m) {
  case OPT_SELF:
    return t->self;
  case OPT_PARENT:
    return t->parent;
  case OPT_LHS:
    return t->lhs;
  case OPT_RHS:
    return t->rhs;
  default:
    assert(false);
  }
}

bool tree_node_equals(local_tree_t *t, uint8_t m, frequency_t f) {
  return (t->opt & m) && tree_select(t, m) == f;
}

void update_single_tree_node(local_tree_t *t, frequency_t old,
                             frequency_t new) {
  if (tree_node_equals(t, OPT_SELF, old)) {
    t->self = new;
  }

  if (tree_node_equals(t, OPT_PARENT, old)) {
    t->parent = new;
  }

  if (tree_node_equals(t, OPT_LHS, old)) {
    t->lhs = new;
  }

  if (tree_node_equals(t, OPT_RHS, old)) {
    t->rhs = new;
  }
}

void swap_tree_state(local_tree_t *t) {
  assert(gs.tree.opt & OPT_SELF);
  assert(t->opt & OPT_SELF);

  gs.tree.opt = t->opt;

  if (tree_node_equals(t, OPT_PARENT, gs.tree.self)) {
    update_single_tree_node(&gs.tree, gs.tree.parent, t->self);
    update_single_tree_node(&gs.tree, gs.tree.lhs, t->lhs);
    update_single_tree_node(&gs.tree, gs.tree.rhs, t->rhs);
  } else if (tree_node_equals(t, OPT_LHS, gs.tree.self)) {
    update_single_tree_node(&gs.tree, gs.tree.parent, t->parent);
    update_single_tree_node(&gs.tree, gs.tree.lhs, t->self);
    update_single_tree_node(&gs.tree, gs.tree.rhs, t->rhs);
  } else if (tree_node_equals(t, OPT_RHS, gs.tree.self)) {
    update_single_tree_node(&gs.tree, gs.tree.parent, t->parent);
    update_single_tree_node(&gs.tree, gs.tree.lhs, t->lhs);
    update_single_tree_node(&gs.tree, gs.tree.rhs, t->self);
  }
}

void update_local_frequencies(frequency_t old, frequency_t new) {
  message_t update_msg;
  update_msg.header.action = DO;
  update_msg.header.type = UPDATE;
  update_msg.header.receiver_id.layer = leader;
  update_msg.header.sender_id.layer = leader;
  update_msg.payload.update.old = old;
  update_msg.payload.update.updated = new;

  if (new != gs.tree.parent) {
    // NOTE: was soll passieren, wenn Leader auf anderer Frequenz auch aktuell
    // einen SWAP macht? Es braucht auf jeden Fall ein System, um die Änderungen
    // nacheinander ohne Konflikte auszuführen. Wir machen es uns jetzt erstmal
    // einfach, indem wir sagen, dass diese Fälle nicht passieren.
    transport_change_frequency(gs.tree.parent);
    transport_send_message(&update_msg);
  }

  if (new != gs.tree.lhs) {
    transport_change_frequency(gs.tree.parent);
    transport_send_message(&update_msg);
  }

  if (new != gs.tree.rhs) {
    transport_change_frequency(gs.tree.parent);
    transport_send_message(&update_msg);
  }

  transport_change_frequency(gs.tree.self);
}

void reject_do_swap() {
  message_t answer;
  answer.header.action = WONT;
  answer.header.type = SWAP;
  // FIXME: Die IDs müssen genauer gesetzt werden, im VIRTUAL Modus
  // kann es sonst sein, dass wir unsere eigene Nachricht bekommen.
  answer.header.receiver_id.layer = leader;
  answer.header.sender_id.layer = leader;
  answer.payload.swap.tree = gs.tree;
  answer.payload.swap.activity_score = gs.scores.current;

  transport_send_message(&answer);
}

// FIXME: braucht vllt Variable, in der steht, an welche Frequenz Swapping
// angefragt wurde
bool handle_do_swap(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == SWAP);

  if (!(gs.flags & LEADER)) {
    fprintf(stderr, "Only leaders should be able to swap, ignoring.\n");
    return false;
  }

  // FIXME: hier müssen irgendwo noch alle nonleader gemutet werden. Macht das
  // der Antragssteller, nachdem er auf Frequenz wechselt oder macht man das
  // selbst?
  if (gs.flags & TREE_SWAPPING) {
    fprintf(stderr, "Received DO SWAP while in the middle of another tree "
                    "operation, ignoring.\n");
    reject_do_swap();
    // NOTE: muss hier false oder true zurückgegeben werden?
    return true;
  }

  if (msg->header.sender_id.layer != leader) {
    fprintf(stderr, "Received DO SWAP from non-leader, ignoring.\n");
    return false;
  }

  if (!(msg->payload.swap.tree.opt & OPT_SELF)) {
    fprintf(stderr,
            "Received DO SWAP doesn't have source frequency set, ignoring.\n");
    return false;
  }

  frequency_t source = msg->payload.swap.tree.self;
  if (source != gs.tree.parent && source != gs.tree.lhs && source != gs.tree.rhs) {
    fprintf(stderr, "Received DO SWAP from a frequency that is not part of my "
                    "local tree, ignoring.\n");
    return false;
  }

  // NOTE: eigener Activity Score muss immer noch irgendwo berechnet werden.
  uint8_t score = msg->payload.swap.activity_score;
  if ((score <= gs.scores.current && source != gs.tree.parent) ||
      (score >= gs.scores.current && source == gs.tree.parent)) {
    fprintf(stderr,
            "Tree Order is still preserved, I see no reason to swap!\n");
    reject_do_swap();
    return true;
  }

  // FIXME: Diese Funktionen sollten auf jeden Fall klarer benannt werden
  update_local_frequencies(gs.tree.self, source);
  swap_tree_state(&msg->payload.swap.tree);
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

  if (msg->header.sender_id.layer != leader) {
    fprintf(stderr, "Received WILL SWAP from non-leader, ignoring.\n");
    return false;
  }

  frequency_t source = msg->payload.swap.tree.self;
  if (source != gs.tree.parent && source != gs.tree.lhs && source != gs.tree.rhs) {
    fprintf(stderr,
            "Received WILL SWAP from a frequency that is not part of my "
            "local tree, ignoring.\n");
    return false;
  }

  update_local_frequencies(gs.tree.self, source);
  swap_tree_state(&msg->payload.swap.tree);

  gs.flags &= ~TREE_SWAPPING;
  return true;
}

bool handle_wont_swap(message_t *msg) {
  assert(message_action(msg) == WONT);
  assert(message_type(msg) == SWAP);

  gs.flags &= ~TREE_SWAPPING;
  return true;
}

// FIXME: braucht globale Report Timestamp
// wenn es normalerweise alle N Zeitschritte ein DO REPORT gibt, kann man
// eigentlich davon ausgehen, dass der Leader nach 1.5 * N + random()
// Zeitschritten inaktiv ist. In dem Fall sollte ein neuer Leader gewählt
// werden. Sobald bei einem Node dieser Fall eintritt, sollte er die folgenden
// Schritte ausführen:
// 1) LEADER Flag setzen
// 2) DO REPORT an alle verschicken
// 3) Next Scheduled Report Timestamp auf aktuelle Timestamp + N setzen
//
// FIXME: Was ist wenn sich mehrere Nodes gleichzeitig zum Leader ernennen?
// Um das Problem zu lösen, kann 1.5 * N + random() mit dem DO REPORT Paket
// verschickt werden. Wer den kleinsten Wert hat, wird zum Leader (wir gehen
// hier mal davon aus, dass die verschwindend kleine Chance, dass die beiden
// Werte gleich sind, nicht vorkommt). Das ist auch cool, weil dann variabel
// gesteuert werden kann, wie oft ein REPORT verschickt wird.
// Das würde ungefähr so ablaufen:
// 1) Leader verschickt Report mit N = 10000, setzt Next Interval auf N = 10000
// 2) Nodes setzen Next Interval auf 1.5 * N + random() = 15000 + random()
// 3) Leader verschickt danach Report mit N = 5000
// 4) Nodes setzen Next Interval auf 1.5 * N + random() = 7500 + random()
// Der Leader kann so das Report Interval selbst festlegen, ohne Gefahr zu
// laufen, dass er deswegen gestürzt wird ;)
bool handle_do_report(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == REPORT);

  // TODO:
  // 1) Next Interval zurücksetzen (aktuelle Zeit + 1.5 * N +
  // random())
  // 2) Mit WILL REPORT antworten

  return true;
}

bool handle_will_report(message_t *msg) {
  assert(message_action(msg) == WILL);
  assert(message_type(msg) == REPORT);

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

    if (tree_node_equals(&gs.tree, OPT_SELF, f)) {
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

  if (!(gs.flags & REGISTERED)){
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

  // leader informs where node might be found
  if (!searching_for_self) {
    reply_msg.payload.find.frequencies = gs.tree;
  }

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
    return true;
  } else {
    assert(sender.layer == leader);
    expand_search_queue(msg->payload.find.frequencies);
  }

  return true;
}

bool handle_message(message_t *msg) {
  assert(message_is_valid(msg));

  return message_handlers[message_action(msg)][message_type(msg)](msg);
}
