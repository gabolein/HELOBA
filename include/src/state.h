#ifndef STATE_H
#define STATE_H

#include "lib/datastructures/generic/generic_hashmap.h"
#include "src/protocol/message.h"
#include "src/protocol/search.h"
#include "src/protocol/routing.h"

MAKE_SPECIFIC_VECTOR_HEADER(routing_id_t, routing_id_t)
MAKE_SPECIFIC_HASHMAP_HEADER(routing_id_t, bool, club)

typedef enum {
  LEADER = 1 << 0,
  TREE_SWAPPING = 1 << 1,
  MUTED = 1 << 2,
  TRANSFERING = 1 << 3,
  SEARCHING = 1 << 4,
  REGISTERED = 1 << 5
} flags_t;

typedef struct {
  uint8_t previous;
  uint8_t current;
} score_state_t;

typedef struct {
  frequency_t old;
  struct timespec last_migrate;
} migrate_state_t;

typedef struct {
  club_hashmap_t *members;
  score_state_t scores;
  routing_id_t id;
  flags_t flags;
  frequency_t frequency;
  search_state_t search;
  migrate_state_t migrate;
} state_t;

state_t gs;
void initialize_global_state();

#endif
