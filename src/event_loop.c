#include "src/protocol/message_handler.h"
#include "src/transport.h"

void event_loop_run() {
  message_t received;
  if (transport_receive_message(&received)) {
    handle_message(&received);
  }

  // if command from interface
  //   execute command
}