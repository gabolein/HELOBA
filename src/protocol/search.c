#include "src/protocol/search.h"
#include "src/protocol/message.h"
#include "src/protocol/message_processor.h"
#include "src/protocol/message_handler.h"
#include "src/transport.h"
#include "lib/time_util.h"
#include <time.h>

#define DO_FIND_SEND_TIMEOUT 1000

static search_state_t global_search_state;

// TODO prefer parent frequencies
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
  search_frequencies_priority_queue_destroy(global_search_state.search_frequencies);
  global_search_state.search_frequencies = search_frequencies_priority_queue_create();
  return true;
}

routing_id_t get_to_find(){
  assert(global_search_state.searching);
  return global_search_state.to_find_id;
}

bool send_do_find(){
  routing_id_t self_id;
  self_id.layer = nonleader;
  routing_id_t receiver_id = {.layer = everyone};
  get_id(self_id.optional_MAC);
  message_t do_find_msg = { .header = {DO, FIND, self_id, receiver_id}, 
    .payload.find.to_find = global_search_state.to_find_id};

  struct timespec start_time;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
  while(!hit_timeout(DO_FIND_SEND_TIMEOUT, &start_time)){
    if(transport_send_message(do_find_msg)){
      return true;
    }
  }
  return false;
}


bool wait_will_do(){
  message_t msg;
  struct timespec start_time;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
  while(!hit_timeout(DO_FIND_SEND_TIMEOUT, &start_time)){
    if(transport_receive_message(&msg) && process_message(&msg)){
      // NOTE assuming we enter the will find handler here
      handle_message(&msg);
      return true;
    }
  }
  return false;
}

bool search_for(routing_id_t to_find){
  assert(!global_search_state.searching);
  global_search_state.searching = true;
  global_search_state.to_find_id = to_find;
  // TODO unregister from frequency

  while(global_search_state.searching){
    
    // TODO what if node is not anywhere? would be stuck in frequency
    if(search_frequencies_priority_queue_size(global_search_state.search_frequencies)){
      frequency_t next_frequency = search_frequencies_priority_queue_pop(global_search_state.search_frequencies);
      change_frequency(next_frequency);
    }

    if(!send_do_find()){
      return false;
    }

    if(!wait_will_do()){
      search_concluded();
      return false;
    }
  }

  return true;
}
