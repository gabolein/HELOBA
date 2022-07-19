#include "src/protocol/message_formatter.h"
#define LOG_LEVEL DEBUG_LEVEL
#define LOG_LABEL "Event Loop"

#include "lib/logger.h"
#include "src/interface/interface.h"
#include "src/protocol/message.h"
#include "src/protocol/search.h"
#include "src/protocol/swap.h"
#include "src/protocol/transfer.h"
#include "src/protocol/tree.h"
#include "src/state.h"
#include "src/transport.h"
#include <stdint.h>

typedef bool (*handler_f)(message_t *msg);
handler_f auto_handlers[MESSAGE_ACTION_COUNT][MESSAGE_TYPE_COUNT];

void event_loop_initialize() {
  memset(auto_handlers, 0, sizeof(auto_handlers));

  // FIXME: auto_handlers als Parameter übergeben, anstatt in den einzelnen
  // Dateien als extern zu deklarieren
  register_automatic_transfer_handlers();
  register_automatic_swap_handlers();
  register_automatic_search_handlers();

  // NOTE: können wir im Event Loop irgendwie in einem Fall landen, in dem wir
  // nicht registriert sind? Oder wird das alles von den Handlers übernommen?
  transport_change_frequency(800);
  perform_registration(800);
}

void event_loop_run() {
  message_t received;

  if (transport_receive_message(&received)) {
    if (auto_handlers[message_action(&received)][message_type(&received)] ==
        NULL) {
      warnln("No automatic handler registered for received message:\n%s",
             format_message(&received));
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
