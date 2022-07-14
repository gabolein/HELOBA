#include "src/protocol/message_handler.h"
#include "src/transport.h"
#include "src/interface/interface.h"
#include "src/state.h"

void event_loop_run() {
  message_t received;
  if (transport_receive_message(&received)) {
    handle_message(&received);
  }

  interface_do_action();

  if (gs.flags & LEADER) {
    // TODO check how many nodes
    // more than threshold z? split to children
    // more than before by factor x.1? swap with parent
    // less than before by factor x.2? swap with child
  }
}
