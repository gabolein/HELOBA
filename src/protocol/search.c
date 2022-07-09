#include "src/protocol/search.h"

static search_state_t global_search_state;

int frequency_cmp(__attribute__((unused))frequency_t a, __attribute__((unused))frequency_t b){
  return 0;
}

MAKE_SPECIFIC_VECTOR_SOURCE(frequency_t, search_frequencies);
MAKE_SPECIFIC_PRIORITY_QUEUE_SOURCE(frequency_t, search_frequencies, frequency_cmp)

void search_frequencies_queue_add(frequency_t freq){
  search_frequencies_priority_queue_push(global_search_state.search_frequencies, freq);
}

void search_state_initialize(){
  global_search_state.searching = false;
  global_search_state.search_frequencies = search_frequencies_priority_queue_create();
}

bool search_concluded(){
  global_search_state.searching = false;
  return true;
}

routing_id_t get_to_find(){
  assert(global_search_state.searching);
  return global_search_state.to_find_id;
}
