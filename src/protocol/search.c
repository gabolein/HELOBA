#define LOG_LEVEL DEBUG_LEVEL
#define LOG_LABEL "Search"

#include "src/protocol/search.h"
#include "lib/datastructures/generic/generic_hashmap.h"
#include "lib/logger.h"
#include "lib/time_util.h"
#include "src/config.h"
#include "src/protocol/cache.h"
#include "src/protocol/message.h"
#include "src/protocol/message_formatter.h"
#include "src/protocol/message_util.h"
#include "src/protocol/transfer.h"
#include "src/protocol/tree.h"
#include "src/state.h"
#include "src/transport.h"
#include <time.h>

extern handler_f auto_handlers[MESSAGE_ACTION_COUNT][MESSAGE_TYPE_COUNT];

void register_automatic_search_handlers() {
  auto_handlers[DO][FIND] = handle_do_find;
}

int hint_cmp(search_hint_t a, search_hint_t b) {
  if (a.type == CACHE && b.type == ORDER) {
    return 1;
  }

  if (a.type == ORDER && b.type == CACHE) {
    return -1;
  }

  if (a.type == CACHE && b.type == CACHE) {
    if (a.timedelta_us < b.timedelta_us) {
      return 1;
    } else if (b.timedelta_us < a.timedelta_us) {
      return -1;
    }
  }

  if (a.type == ORDER && b.type == ORDER) {
    if (tree_node_gt(a.f, b.f)) {
      return 1;
    } else {
      return -1;
    }
  }

  return 0;
}

bool frequency_eq(frequency_t a, frequency_t b) { return a == b; }

MAKE_SPECIFIC_VECTOR_SOURCE(search_hint_t, search);
MAKE_SPECIFIC_PRIORITY_QUEUE_SOURCE(search_hint_t, search, hint_cmp)

MAKE_SPECIFIC_VECTOR_SOURCE(frequency_t, checked_key)
MAKE_SPECIFIC_HASHMAP_SOURCE(frequency_t, bool, checked, frequency_eq)

void search_queue_add(search_hint_t hint) {
  if (checked_hashmap_exists(gs.search.checked_frequencies, hint.f)) {
    return;
  }

  search_priority_queue_push(gs.search.search_queue, hint);
}

// TOOD: besseren Namen finden
void search_queue_expand_by_order(frequency_t f) {
  frequency_t next_freqs[3] = {
      tree_node_parent(f),
      tree_node_lhs(f),
      tree_node_rhs(f),
  };

  for (unsigned i = 0; i < 3; i++) {
    if (next_freqs[i] == f) {
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
  gs.search.search_queue = search_priority_queue_create();
  gs.search.checked_frequencies = checked_hashmap_create();
}

bool search_response_filter(message_t *msg) {
  return message_action(msg) == WILL &&
         (message_type(msg) == FIND || message_type(msg) == HINT);
}

bool perform_search(routing_id_t to_find, frequency_t *found) {
  checked_hashmap_clear(gs.search.checked_frequencies);
  search_priority_queue_clear(gs.search.search_queue);

  if (cache_hit(to_find)) {
    search_hint_t cache_hint = {
        .type = CACHE,
        .f = cache_get(to_find).f,
        .timedelta_us = cache_get(to_find).timedelta_us,
    };
    search_queue_add(cache_hint);
  }

  frequency_t f = gs.frequency;
  search_hint_t start = {
      .type = ORDER,
      .f = f,
  };
  search_queue_add(start);

  while (search_priority_queue_size(gs.search.search_queue) > 0) {
    search_hint_t next_hint = search_priority_queue_pop(gs.search.search_queue);
    if (checked_hashmap_exists(gs.search.checked_frequencies, next_hint.f)) {
      continue;
    }

    f = next_hint.f;
    transport_change_frequency(f);
    checked_hashmap_insert(gs.search.checked_frequencies, f, true);

    message_t scan = message_create(DO, FIND);
    scan.payload.find = (find_payload_t){.to_find = to_find};
    routing_id_t receivers = routing_id_create(everyone, NULL);
    transport_send_message(&scan, receivers);

    message_vector_t *responses = message_vector_create();
    collect_messages(FIND_RESPONSE_TIMEOUT_MS, 5, search_response_filter,
                     responses);

    for (unsigned i = 0; i < message_vector_size(responses); i++) {
      message_t current = message_vector_at(responses, i);

      if (message_type(&current) == FIND) {
        // NOTE: Bei anderen Suchvarianten, z.B. nach Hash wird dieser Check
        // nicht gebraucht
        if (!routing_id_equal(current.header.sender_id, to_find)) {
          warnln("Got FIND response from Node we didn't search.");
          continue;
        }

        cache_insert(current.header.sender_id, f);
        *found = f;
        dbgln("Took %u hops to find Node %s",
              checked_hashmap_size(gs.search.checked_frequencies) - 1,
              format_routing_id(to_find));
        message_vector_destroy(responses);
        return true;
      } else if (message_type(&current) == HINT) {
        search_hint_t hint = {
            .type = CACHE,
            .f = current.payload.hint.hint.f,
            .timedelta_us = current.payload.hint.hint.timedelta_us,
        };

        search_queue_add(hint);
      }
    }

    message_vector_destroy(responses);
    search_queue_expand_by_order(f);
  }

  // if we really didn't find anyone, we should have checked all frequencies
  assert(checked_hashmap_size(gs.search.checked_frequencies) ==
         FREQUENCY_CEILING - FREQUENCY_BASE + 1);
  return false;
}

bool handle_do_find(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == FIND);

  if (!gs.registered) {
    return false;
  }

  routing_id_t to_find = msg->payload.find.to_find;
  if (!routing_id_equal(to_find, gs.id)) {
    if (cache_hit(to_find)) {
      cache_hint_t hint = cache_get(to_find);
      if (hint.f == gs.frequency) {
        return true;
      }

      message_t reply = message_create(WILL, HINT);
      reply.payload.hint = (hint_payload_t){.hint = hint};
      transport_send_message(&reply, msg->header.sender_id);
    }
  } else {
    message_t reply = message_create(WILL, FIND);
    // NOTE pack routing id checks for specific bit
    reply.payload.find.to_find.layer = everyone;
    transport_send_message(&reply, msg->header.sender_id);
  }

  return true;
}
