#define LOG_LEVEL WARNING_LEVEL
#define LOG_LABEL "Event Loop"

#include "lib/datastructures/generic/generic_vector.h"
#include "lib/datastructures/int_vector.h"
#include "lib/datastructures/u8_vector.h"
#include "lib/logger.h"
#include "lib/random.h"
#include "lib/time_util.h"
#include "src/config.h"
#include "src/interface/interface.h"
#include "src/protocol/message.h"
#include "src/protocol/message_formatter.h"
#include "src/protocol/search.h"
#include "src/protocol/swap.h"
#include "src/protocol/transfer.h"
#include "src/protocol/tree.h"
#include "src/state.h"
#include "src/transport.h"
#include <stdint.h>
#include <stdio.h>
#include <time.h>

typedef bool (*handler_f)(message_t *msg);
handler_f auto_handlers[MESSAGE_ACTION_COUNT][MESSAGE_TYPE_COUNT];

static unsigned timeout_ms;
static struct timespec timeout_start;

// NOTE: nimmt an, dass im VIRTUAL Mode getestet wird. Das ist eine nervige
// Restriktion, weil wir im Moment nur Suche nach ID unterstützen und kein
// weiteres Overlay Protokoll haben, was Suche nach Hash, Gruppe, etc
// ermöglicht.
// NOTE: Anscheinend verursacht fread() in /usr/bin/killall5 Memory Leaks,
// dagegen können wir nichts tun, deswegen Instrumentation für diese Funktion
// ausschalten.
__attribute__((no_sanitize_address)) routing_id_t get_random_network_node() {
  unsigned char buf[256];
  u8_vector_t *pidof_output = u8_vector_create();
  FILE *pipe = popen("pidof heloba", "r");

  size_t ret;
  while ((ret = fread(buf, 1, sizeof(buf), pipe)) > 0) {
    for (unsigned i = 0; i < ret; i++) {
      u8_vector_append(pidof_output, buf[i]);
    }
  }
  pclose(pipe);

  int_vector_t *converted_pids = int_vector_create();
  int cpid = 0;
  for (unsigned i = 0; i < u8_vector_size(pidof_output); i++) {
    uint8_t c = u8_vector_at(pidof_output, i);

    if (c == ' ' || i == u8_vector_size(pidof_output) - 1) {
      int_vector_append(converted_pids, cpid);
      cpid = 0;
      continue;
    }

    cpid = 10 * cpid + (c - '0');
  }

  if (int_vector_size(converted_pids) == 0) {
    return gs.id;
  }

  unsigned rd = random_number_between(0, int_vector_size(converted_pids));
  pid_t chosen = int_vector_at(converted_pids, rd);

  u8_vector_destroy(pidof_output);
  int_vector_destroy(converted_pids);

  uint8_t MAC[MAC_SIZE];
  memset(MAC, 0, MAC_SIZE);
  memcpy(MAC, &chosen, sizeof(chosen));
  return routing_id_create(specific, MAC);
}

void event_loop_initialize() {
  memset(auto_handlers, 0, sizeof(auto_handlers));

  // FIXME: auto_handlers als Parameter übergeben, anstatt in den einzelnen
  // Dateien als extern zu deklarieren
  register_automatic_transfer_handlers();
  register_automatic_swap_handlers();
  register_automatic_search_handlers();

  // NOTE: können wir im Event Loop irgendwie in einem Fall landen, in dem wir
  // nicht registriert sind? Oder wird das alles von den Handlers übernommen?
  transport_change_frequency(FREQUENCY_BASE);
  perform_registration(FREQUENCY_BASE);

  timeout_ms = random_number_between(250, 1000);
  clock_gettime(CLOCK_MONOTONIC_RAW, &timeout_start);
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

    // TODO: Ein paar von den Auto Handlers sollten nicht aufgerufen werden, je
    // nachdem ob man registered oder leader ist.
    auto_handlers[message_action(&received)][message_type(&received)](
        &received);
  }

  interface_do_action();

  if (gs.id.layer & leader) {
    balance_frequency();
  }

  if (hit_timeout(timeout_ms, &timeout_start) && !(gs.id.layer & leader)) {
    routing_id_t target = get_random_network_node();
    if (routing_id_equal(target, gs.id)) {
      timeout_ms = random_number_between(250, 1000);
      clock_gettime(CLOCK_MONOTONIC_RAW, &timeout_start);
      return;
    }

    frequency_t found;

    if (!perform_search(target, &found)) {
      warnln("Couldn't find requested node %s", format_routing_id(target));
      timeout_ms = random_number_between(250, 1000);
      clock_gettime(CLOCK_MONOTONIC_RAW, &timeout_start);
      return;
    }

    if (found != gs.frequency) {
      transport_change_frequency(gs.frequency);
      perform_unregistration(found);
      transport_change_frequency(found);
      perform_registration(found);
    }

    timeout_ms = random_number_between(250, 1000);
    clock_gettime(CLOCK_MONOTONIC_RAW, &timeout_start);
  }
}
