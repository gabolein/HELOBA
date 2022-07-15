#include "src/interface/interface.h"
#include "src/protocol/message.h"
#include "src/protocol/message_handler.h"
#include "src/protocol/routing.h"
#include "src/protocol/tree.h"
#include "src/protocol/transfer.h"
#include "src/state.h"
#include "src/transport.h"
#include <stdint.h>

#define MIN_SPLIT_SCORE 5
#define MIN_SWAP_SCORE 10
#define MIN_LT_SWAP_RATIO -1.25
#define MIN_GT_SWAP_RATIO 1.25

float score_trajectory(uint8_t old, uint8_t new) {
  float ratio = (float)new / (float)old;
  if (ratio >= 1.0) {
    return ratio;
  }

  return -((float)old / (float)new);
}

bool swap_reply_filter(message_t *msg) {
  return (message_action(msg) == WILL || message_action(msg) == WONT) &&
         message_type(msg) == SWAP;
}

bool perform_swap(frequency_t with) {
  if (!is_valid_tree_node(with)) {
    fprintf(stderr, "Can't swap with frequency %u, is invalid.\n", with);
    return false;
  }

  transport_change_frequency(with);

  message_t swap_start = message_create(DO, SWAP);
  swap_start.payload.swap = (swap_payload_t){
      .score = gs.scores.current,
      .with = gs.frequency,
  };
  routing_id_t receiver = {.layer = leader};
  transport_send_message(&swap_start, receiver);

  message_vector_t *replies = message_vector_create();
  message_assign_collector(10, 1, swap_reply_filter, replies);

  if (message_vector_empty(replies)) {
    fprintf(stderr,
            "Didn't receive answer from frequency %u, assuming failure.\n",
            with);
    message_vector_destroy(replies);
    return false;
  }

  message_t reply = message_vector_at(replies, 0);
  if (message_action(&reply) == WONT) {
    fprintf(stderr, "Frequency %u doesn't want to swap, aborting.\n", with);
    message_vector_destroy(replies);
    return false;
  }

  // TODO: SWAP auf unserer Seite durchführen
  return true;
}

void event_loop_run() {
  message_t received;
  if (transport_receive_message(&received)) {
    handle_message(&received);
  }

  interface_do_action();

  if (gs.flags & LEADER) {
    if (gs.scores.current >= MIN_SPLIT_SCORE &&
        gs.scores.current < MIN_SWAP_SCORE) {
      perform_split();
    }

    if (gs.scores.current >= MIN_SWAP_SCORE) {
      float ratio = score_trajectory(gs.scores.previous, gs.scores.current);
      if (ratio > 0 && ratio > MIN_GT_SWAP_RATIO &&
          gs.frequency != tree_node_parent(gs.frequency)) {
        perform_swap(tree_node_parent(gs.frequency));
        gs.scores.previous = gs.scores.current;
      } else if (ratio < 0 && ratio < MIN_LT_SWAP_RATIO) {
        bool ret = false;
        if (gs.frequency != tree_node_lhs(gs.frequency)) {
          ret = perform_swap(tree_node_lhs(gs.frequency));
        }

        if (!ret && gs.frequency != tree_node_rhs(gs.frequency)) {
          ret = perform_swap(tree_node_rhs(gs.frequency));
        }

        gs.scores.previous = gs.scores.current;
      }
    }
  }
}
