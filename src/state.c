#define LOG_LEVEL DEBUG_LEVEL
#define LOG_LABEL "State"

#include "src/state.h"
#include "src/transport.h"

state_t gs;

MAKE_SPECIFIC_VECTOR_SOURCE(routing_id_t, club_key)
MAKE_SPECIFIC_HASHMAP_SOURCE(routing_id_t, bool, club, routing_id_equal)

void initialize_global_state() {
  gs.registered = false;
  score_state_t scores = {0, 0};
  gs.scores = scores;
  gs.members = club_hashmap_create();
  transport_get_id(gs.id.MAC);
  gs.id.layer = specific;
  search_state_initialize();
}
