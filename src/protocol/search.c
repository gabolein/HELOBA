#include "src/protocol/search.h"
#include "lib/time_util.h"
#include "src/protocol/message.h"
#include "src/protocol/message_handler.h"
#include "src/protocol/message_processor.h"
#include "src/transport.h"
#include "src/state.h"
#include <time.h>

#define DO_FIND_SEND_TIMEOUT 1000

// TODO prefer parent frequencies
int frequency_cmp(__attribute__((unused)) frequency_t a,
                  __attribute__((unused)) frequency_t b) {
  return 0;
}

MAKE_SPECIFIC_VECTOR_SOURCE(frequency_t, search_frequencies);
MAKE_SPECIFIC_PRIORITY_QUEUE_SOURCE(frequency_t, search_frequencies,
                                    frequency_cmp)

void search_frequencies_queue_add(frequency_t freq) {
  search_frequencies_priority_queue_push(gs.search.search_frequencies,
                                         freq);
}

void search_state_initialize() {
  gs.flags &= ~(SEARCHING);
  gs.search.search_frequencies =
      search_frequencies_priority_queue_create();
}

bool search_concluded() {
  gs.flags &= ~(SEARCHING);
  search_frequencies_priority_queue_destroy(
      gs.search.search_frequencies);
  gs.search.search_frequencies =
      search_frequencies_priority_queue_create();
  // TODO register in frequency
  return true;
}

void expand_search_queue(local_tree_t tree){
  if(tree.opt & OPT_PARENT){
    search_frequencies_priority_queue_push(gs.search.search_frequencies,
        tree.parent);
  }

  if(tree.opt & OPT_LHS){
    search_frequencies_priority_queue_push(gs.search.search_frequencies, tree.lhs);
  }

  if(tree.opt & OPT_RHS){
    search_frequencies_priority_queue_push(gs.search.search_frequencies, tree.rhs);
  }
}

routing_id_t get_to_find() {
  assert(gs.flags & SEARCHING);
  return gs.search.to_find_id;
}

bool send_do_find() {
  routing_id_t self_id;
  self_id.layer = nonleader;
  routing_id_t receiver_id = {.layer = everyone};
  transport_get_id(self_id.optional_MAC);
  message_t do_find_msg = {.header = {DO, FIND, self_id, receiver_id},
                           .payload.find.to_find =
                               gs.search.to_find_id};

  return transport_send_message(&do_find_msg);
}

bool wait_will_do() {
  message_t msg;
  struct timespec start_time;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
  while (!hit_timeout(DO_FIND_SEND_TIMEOUT, &start_time)) {
    if (transport_receive_message(&msg) && process_message(&msg)) {
      // NOTE assuming we enter the will find handler here
      // TODO set state variables such that process message can filter wrong message type
      handle_message(&msg);
      return true;
    }
  }
  return false;
}

bool search_for(routing_id_t to_find) {
  assert(!(gs.flags & SEARCHING));
  gs.flags |= SEARCHING;
  gs.search.to_find_id = to_find;
  // TODO unregister from frequency
  bool found;
  do {
    if (search_frequencies_priority_queue_size(
            gs.search.search_frequencies)) {
      frequency_t next_frequency = search_frequencies_priority_queue_pop(
          gs.search.search_frequencies);
      transport_change_frequency(next_frequency);
    }

    if (!send_do_find()) {
      goto failure;
    }

    if (!wait_will_do()) {
      goto failure;
    }

  } while ((found = gs.flags & SEARCHING) 
      && search_frequencies_priority_queue_size(
        gs.search.search_frequencies));
  

  if(found){
    return true;
  } else {
    goto failure;
  }
  
failure:
  search_concluded();
  return false;
}
