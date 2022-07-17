#define LOG_LEVEL DEBUG_LEVEL
#define LOG_LABEL "Event Loop"

#include "lib/logger.h"
#include "src/interface/interface.h"
#include "src/protocol/message.h"
#include "src/protocol/message_handler.h"
#include "src/protocol/search.h"
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

void balance_frequency() {
  if (gs.scores.current < MIN_SPLIT_SCORE) {
    return;
  }

  if (gs.scores.current >= MIN_SPLIT_SCORE &&
      gs.scores.current < MIN_SWAP_SCORE) {
    dbgln("There are currently %u Nodes on frequency %u, attempting to split.",
          gs.scores.current, gs.frequencies.current);
    perform_split();
  }

  // FIXME: Diese ganzen Conditions müssen überarbeitet werden, dieser Fall z.B.
  // kommt kaum vor, erst wenn SPLIT nicht mehr möglich ist. SPLIT ist auch
  // nicht besonders gut für den Throughput im Netzwerk, weil geclusterte Nodes,
  // die oft miteinander reden, auseinander gebrochen werden.
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

typedef bool (*handler_f)(message_t *msg);
static handler_f auto_handlers[MESSAGE_ACTION_COUNT][MESSAGE_TYPE_COUNT];

void event_loop_initialize() {
  memset(auto_handlers, 0, sizeof(auto_handlers));

  register_automatic_transfer_handlers();
  register_automatic_swap_handlers();
  register_automatic_search_handlers();

  // NOTE: können wir im Event Loop irgendwie in einem Fall landen, in dem wir
  // nicht registriert sind? Oder wird das alles von den Handlers übernommen?
  perform_registration();
}

void event_loop_run() {
  message_t received;

  if (transport_receive_message(&received)) {
    if (auto_handlers[message_action(&received)][message_type(&received)] ==
        NULL) {
      warnln("No automatic handler registered for received message!");
      return;
    }

    auto_handlers[message_action(&received)][message_type(&received)](
        &received);
  }

  interface_do_action();

  if (gs.id.layer & leader) {
    balance_frequency();
  }
}
