#ifndef SEARCH_H
#define SEARCH_H

#include "lib/datastructures/generic/generic_hashmap.h"
#include "lib/datastructures/generic/generic_priority_queue.h"
#include "src/protocol/message.h"
#include "src/protocol/routing.h"

typedef enum { CACHE, ORDER } search_hint_type_t;
typedef enum { UP, DOWN } search_direction_t;

// TODO: wir könnten auch Zeitdelta mitschicken, dann könnte man noch nach
// Neuheit der Cache Einträge sortieren
typedef struct {
  search_hint_type_t type;
  frequency_t f;
} search_hint_t;

// TODO: bessere Namen finden
MAKE_SPECIFIC_PRIORITY_QUEUE_HEADER(search_hint_t, search)
// FIXME: Das mit der keys Funktion geht so nicht, viel zu viel Boilerplate
MAKE_SPECIFIC_VECTOR_HEADER(frequency_t, frequency_t)

MAKE_SPECIFIC_HASHMAP_HEADER(frequency_t, bool, checked)

typedef struct {
  search_direction_t direction;
  frequency_t current_frequency;
  routing_id_t to_find_id;
  bool found;
  search_priority_queue_t *search_queue;
  checked_hashmap_t *checked_frequencies;
} search_state_t;

void search_queue_add(search_hint_t hint);
void search_state_initialize(void);
bool search_concluded(void);
routing_id_t get_to_find(void);
bool search_for(routing_id_t);

#endif
