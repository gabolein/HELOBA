#ifndef SEARCH_H
#define SEARCH_H

#include "lib/datastructures/generic/generic_hashmap.h"
#include "lib/datastructures/generic/generic_priority_queue.h"
#include "src/protocol/message.h"

typedef enum { CACHE, ORDER } search_hint_type_t;
typedef struct {
  search_hint_type_t type;
  frequency_t f;
  unsigned timedelta_us;
} search_hint_t;

// TODO: bessere Namen finden
MAKE_SPECIFIC_PRIORITY_QUEUE_HEADER(search_hint_t, search)
MAKE_SPECIFIC_HASHMAP_HEADER(frequency_t, bool, checked)

typedef struct {
  search_priority_queue_t *search_queue;
  checked_hashmap_t *checked_frequencies;
} search_state_t;

typedef bool (*handler_f)(message_t *msg);
void register_automatic_search_handlers();

void search_queue_add(search_hint_t hint);
void search_state_initialize(void);
routing_id_t get_to_find(void);
bool perform_search(routing_id_t to_find, frequency_t *found);
bool handle_do_find(message_t *);

#endif
