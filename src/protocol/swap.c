#define LOG_LEVEL DEBUG_LEVEL
#define LOG_LABEL "Swap"

#include "src/protocol/swap.h"
#include "lib/logger.h"
#include "lib/time_util.h"
#include "src/protocol/message.h"
#include "src/protocol/message_util.h"
#include "src/protocol/transfer.h"
#include "src/protocol/tree.h"
#include "src/state.h"
#include "src/transport.h"

void accept_swap(routing_id_t receiver);
void reject_swap(routing_id_t receiver);
void perform_migrate(frequency_t with);
bool swap_reply_filter(message_t *msg);

extern handler_f auto_handlers[MESSAGE_ACTION_COUNT][MESSAGE_TYPE_COUNT];

void register_automatic_swap_handlers() {
  auto_handlers[DO][SWAP] = handle_do_swap;
  auto_handlers[DO][MIGRATE] = handle_do_migrate;
}

bool handle_do_migrate(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == MIGRATE);

  if (!(msg->header.sender_id.layer & leader)) {
    dbgln("Received DO MIGRATE from non-leader, ignoring.");
    return false;
  }

  if (gs.id.layer & leader) {
    dbgln("Received DO MIGRATE as leader, ignoring.");
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

  if (!transport_change_frequency(destination)) {
    warnln("Could not change to frequency %u.", destination);
    return false;
  }

  gs.frequency = destination;
  return true;
}

void accept_swap(routing_id_t receiver) {
  message_t answer = message_create(WILL, SWAP);
  answer.payload.swap = (swap_payload_t){
      .with = gs.frequency,
      .score = gs.scores.current,
  };

  transport_send_message(&answer, receiver);
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

void perform_migrate(frequency_t with) {
  message_t migrate = message_create(DO, MIGRATE);
  migrate.payload.transfer = (transfer_payload_t){
      .to = with,
  };
  routing_id_t receivers = routing_id_create(everyone, NULL);
  transport_send_message(&migrate, receivers);
  transport_change_frequency(with);
  gs.frequency = with;
}

bool handle_do_swap(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == SWAP);

  if (!(gs.id.layer & leader) || !(msg->header.sender_id.layer & leader)) {
    dbgln("Only leaders should be able to swap, ignoring.");
    return false;
  }

  frequency_t f = msg->payload.swap.with;
  if (!is_valid_tree_node(f) || f == gs.frequency) {
    dbgln("Sent frequency is invalid, ignoring.");
    reject_swap(msg->header.sender_id);
    return false;
  }

  uint8_t score = msg->payload.swap.score;
  if ((score <= gs.scores.current && tree_node_gt(gs.frequency, f)) ||
      (score >= gs.scores.current && tree_node_gt(f, gs.frequency))) {
    dbgln("Tree Order is still preserved, I see no reason to swap.");
    reject_swap(msg->header.sender_id);
    return true;
  }

  accept_swap(msg->header.sender_id);
  perform_migrate(f);

  return true;
}

bool swap_reply_filter(message_t *msg) {
  return (message_action(msg) == WILL || message_action(msg) == WONT) &&
         message_type(msg) == SWAP;
}

swap_return_val perform_swap(frequency_t with) {
  if (!is_valid_tree_node(with)) {
    warnln("Can't swap with frequency %u, is invalid.", with);
    return false;
  }

  message_t swap_start = message_create(DO, SWAP);
  swap_start.payload.swap = (swap_payload_t){
      .score = gs.scores.current,
      .with = gs.frequency,
  };
  routing_id_t receiver = routing_id_create(leader, NULL);

  transport_change_frequency(with);
  // NOTE: Wenn wir alle nonleader überhaupt noch muten wollen, dann sollte das
  // hier gemacht werden.
  transport_send_message(&swap_start, receiver);

  message_vector_t *replies = message_vector_create();
  collect_messages(10, 1, swap_reply_filter, replies);

  if (message_vector_empty(replies)) {
    warnln("Didn't receive answer from frequency %u, assuming missing.", with);
    message_vector_destroy(replies);
    transport_change_frequency(gs.frequency);
    return TIMEOUT;
  }

  message_t reply = message_vector_at(replies, 0);
  if (message_action(&reply) == WONT) {
    dbgln("Frequency %u doesn't want to swap, aborting.", with);
    message_vector_destroy(replies);
    transport_change_frequency(gs.frequency);
    return REJECTED;
  }

  transport_change_frequency(gs.frequency);
  perform_migrate(with);
  return SUCCESS;
}
