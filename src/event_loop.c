#include "src/interface/interface.h"
#include "src/protocol/message.h"
#include "src/protocol/message_handler.h"
#include "src/protocol/routing.h"
#include "src/protocol/swap.h"
#include "src/protocol/tree.h"
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

void event_loop_run() {
  message_t received;
  if (transport_receive_message(&received)) {
    handle_message(&received);
  }

  interface_do_action();

  if (gs.flags & LEADER) {
    if (gs.scores.current >= MIN_SPLIT_SCORE &&
        gs.scores.current < MIN_SWAP_SCORE) {
      // TODO: split to children
      // 1. sort member list by id
      // 2. send SPLIT message with delimeters sorted[n/4], sorted[2n/4] to
      // everone on frequency
      // 3. receiving nodes will compare id with delimeters
      // 3.1. if id < sorted[n/4] goto lhs
      // 3.2. else if id < sorted[2n/4] goto rhs
      // 3.1. else do nothing
    }

    if (gs.scores.current >= MIN_SWAP_SCORE) {
      frequency_t f = gs.frequencies.current;
      frequency_t parent = tree_node_parent(f), lhs = tree_node_lhs(f),
                  rhs = tree_node_rhs(f);
      float ratio = score_trajectory(gs.scores.previous, gs.scores.current);

      if (ratio > 0 && ratio > MIN_GT_SWAP_RATIO && f != parent) {
        perform_swap(parent);
        gs.scores.previous = gs.scores.current;
      } else if (ratio < 0 && ratio < MIN_LT_SWAP_RATIO) {
        bool ret = false;
        if (f != lhs) {
          ret = perform_swap(lhs);
        }

        if (!ret && f != rhs) {
          ret = perform_swap(rhs);
        }

        gs.scores.previous = gs.scores.current;
      }
    }
  }
}
