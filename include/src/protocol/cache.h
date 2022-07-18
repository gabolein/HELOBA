#ifndef CACHE_H
#define CACHE_H

#include "lib/datastructures/generic/generic_hashmap.h"
#include "src/protocol/message.h"
#include <stdbool.h>
#include <time.h>

// FIXME: Weg finden, diese Informationen nicht anderen Dateien zur Verfügung
// stellen zu müssen.
// NOTE: Könnte vom HashMap Rewrite mit mehreren Vectors + Modularisierung des
// Message Codes gelöst werden.
typedef struct {
  routing_id_t id;
  frequency_t f;
  struct timespec timestamp;
} cache_entry_t;

MAKE_SPECIFIC_HASHMAP_HEADER(routing_id_t, cache_entry_t, rc)

bool cache_hit(routing_id_t id);
rc_key_vector_t *cache_contents();
cache_hint_t cache_get(routing_id_t id);
void cache_remove(routing_id_t id);
void cache_insert(routing_id_t id, frequency_t f);

#endif