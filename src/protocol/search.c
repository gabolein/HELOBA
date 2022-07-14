#include "src/protocol/search.h"
#include "lib/datastructures/generic/generic_hashmap.h"
#include "lib/time_util.h"
#include "src/protocol/message.h"
#include "src/protocol/message_handler.h"
#include "src/protocol/routing.h"
#include "src/protocol/tree.h"
#include "src/state.h"
#include "src/transport.h"
#include <time.h>

// FIXME: viel zu groß
#define DO_FIND_SEND_TIMEOUT 1000
#define WILL_FIND_RECV_TIMEOUT 10

int hint_cmp(search_hint_t a, search_hint_t b) {
  if (a.type == CACHE && b.type == ORDER) {
    return 1;
  }

  if (a.type == ORDER && b.type == CACHE) {
    return -1;
  }

  return 0;
}

bool frequency_eq(frequency_t a, frequency_t b) { return a == b; }

MAKE_SPECIFIC_VECTOR_SOURCE(search_hint_t, search);
MAKE_SPECIFIC_PRIORITY_QUEUE_SOURCE(search_hint_t, search, hint_cmp)

MAKE_SPECIFIC_VECTOR_SOURCE(frequency_t, frequency_t)
MAKE_SPECIFIC_HASHMAP_SOURCE(frequency_t, bool, checked, frequency_eq)

void search_queue_add(search_hint_t hint) {
  if (checked_hashmap_exists(gs.search.checked_frequencies, hint.f)) {
    return;
  }

  search_priority_queue_push(gs.search.search_queue, hint);
}

// TOOD: besseren Namen finden
void search_queue_expand_by_order() {
  if (gs.search.current_frequency ==
          tree_node_parent(gs.search.current_frequency) &&
      gs.search.direction == UP) {
    gs.search.direction = DOWN;
  }

  frequency_t next_freqs[3] = {
      tree_node_parent(gs.search.current_frequency),
      tree_node_lhs(gs.search.current_frequency),
      tree_node_rhs(gs.search.current_frequency),
  };

  for (unsigned i = 0; i < 3; i++) {
    if (next_freqs[i] == gs.search.current_frequency) {
      continue;
    }

    if (gs.search.direction == DOWN &&
        next_freqs[i] == tree_node_parent(gs.search.current_frequency)) {
      continue;
    }

    if (gs.search.direction == UP &&
        next_freqs[i] != tree_node_parent(gs.search.current_frequency)) {
      continue;
    }

    search_hint_t hint = {
        .type = ORDER,
        .f = next_freqs[i],
    };

    search_queue_add(hint);
  }
}

void search_state_initialize() {
  gs.flags &= ~(SEARCHING);
  gs.search.search_queue = search_priority_queue_create();
  gs.search.checked_frequencies = checked_hashmap_create();
}

bool search_concluded() {
  gs.flags &= ~(SEARCHING);
  search_priority_queue_destroy(gs.search.search_queue);
  gs.search.search_queue = search_priority_queue_create();
  // TODO register in frequency
  return true;
}

routing_id_t get_to_find() {
  assert(gs.flags & SEARCHING);
  return gs.search.to_find_id;
}

bool wait_will_find() {
  message_t msg;
  struct timespec start_time;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
  while (!hit_timeout(WILL_FIND_RECV_TIMEOUT, &start_time)) {
    if (transport_receive_message(&msg) && message_action(&msg) == WILL &&
        message_type(&msg) == FIND) {
      // NOTE: vielleicht aus message_handler.c hierher verschieben
      handle_message(&msg);
    }
  }

  return true;
}

bool search_for(routing_id_t to_find) {
  if (!(gs.flags & SEARCHING)) {
    fprintf(stderr, "Currently still searching for another ID, ignoring.\n");
    return false;
  }

  gs.flags |= SEARCHING;
  gs.search.to_find_id = to_find;
  checked_hashmap_clear(gs.search.checked_frequencies);
  gs.search.current_frequency = gs.frequency;
  gs.search.direction = UP;
  gs.search.found = false;
  search_priority_queue_clear(gs.search.search_queue);

  search_hint_t start = {
      .type = ORDER,
      .f = gs.search.current_frequency,
  };
  search_priority_queue_push(gs.search.search_queue, start);

  // TODO unregister from frequency

  while (gs.flags & SEARCHING) {
    if (search_priority_queue_size(gs.search.search_queue) == 0) {
      break;
    }

    search_hint_t next_hint = search_priority_queue_pop(gs.search.search_queue);
    transport_change_frequency(next_hint.f);
    gs.search.current_frequency = next_hint.f;
    checked_hashmap_insert(gs.search.checked_frequencies, next_hint.f, true);

    message_t scan = message_create(DO, FIND);
    scan.payload.find = (find_payload_t){.to_find = gs.search.to_find_id};
    routing_id_t receivers = {.layer = everyone};
    transport_send_message(&scan, receivers);

    wait_will_find();

    search_queue_expand_by_order();
  }

  return gs.search.found;
}
