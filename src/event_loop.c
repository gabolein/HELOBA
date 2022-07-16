#define LOG_LEVEL DEBUG_LEVEL
#define LOG_LABEL "Event Loop"

#include "lib/logger.h"
#include "src/interface/interface.h"
#include "src/protocol/message.h"
#include "src/protocol/message_handler.h"
#include "src/protocol/routing.h"
#include "src/protocol/swap.h"
#include "src/protocol/transfer.h"
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
  if (!(gs.flags & REGISTERED)) {
    if (!perform_registration()) {
      perror("Registration:");
    } else {
      assert(gs.flags & REGISTERED);
    }
  }

  message_t received;
  if (transport_receive_message(&received)) {
    dbgln("Event Loop: received message. Handling...");
    handle_message(&received);
  }

  interface_do_action();

  if (gs.flags & LEADER) {
    if (gs.scores.current >= MIN_SPLIT_SCORE &&
        gs.scores.current < MIN_SWAP_SCORE) {
      dbgln("Many nodes on freq. Splitting ... ");
      perform_split();
    }

    if (gs.scores.current >= MIN_SWAP_SCORE) {
      frequency_t f = gs.frequencies.current;
      frequency_t parent = tree_node_parent(f), lhs = tree_node_lhs(f),
                  rhs = tree_node_rhs(f);
      float ratio = score_trajectory(gs.scores.previous, gs.scores.current);

      if (ratio > 0 && ratio > MIN_GT_SWAP_RATIO && f != parent) {
        dbgln("Try to swap with parent");
        perform_swap(parent);
        gs.scores.previous = gs.scores.current;
      } else if (ratio < 0 && ratio < MIN_LT_SWAP_RATIO) {
        bool ret = false;
        dbgln("Try to swap with child");
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
