#ifndef SEARCH_H
#define SEARCH_H

#include "lib/datastructures/generic/generic_priority_queue.h"
#include "src/protocol/routing.h"
#include "src/protocol/message.h"

#define FREQUENCY_ENCODED_SIZE sizeof(uint16_t)
typedef uint16_t frequency_t;

MAKE_SPECIFIC_PRIORITY_QUEUE_HEADER(frequency_t, search_frequencies)

typedef struct {
  bool searching;
  routing_id_t to_find_id;
  search_frequencies_priority_queue_t *search_frequencies;
} search_state_t;

void search_frequencies_queue_add(frequency_t);
void expand_search_queue(local_tree_t);
void search_state_initialize(void);
bool search_concluded(void);
routing_id_t get_to_find(void);

#endif
