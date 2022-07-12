#include "src/state.h"

bool freq_eq(frequency_t a, frequency_t b) { return a == b; }
MAKE_SPECIFIC_HASHMAP_SOURCE(frequency_t, bool, club, freq_eq)