#include "src/state.h"
#include "src/transport.h"

bool freq_eq(frequency_t a, frequency_t b) { return a == b; }
MAKE_SPECIFIC_HASHMAP_SOURCE(frequency_t, bool, club, freq_eq)

void initialize_global_state(){
  gs.flags = 0;
  score_state_t scores = {0, 0};
  gs.scores = scores;
  gs.members = club_hashmap_create();
  transport_get_id(gs.id.optional_MAC);
  gs.tree.opt = 0;
}


