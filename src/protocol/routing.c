#include "src/protocol/routing.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

bool routing_id_MAC_equal(routing_id_t id1, routing_id_t id2) {
  assert(id1.layer != everyone && id2.layer != everyone);

  for (size_t i = 0; i < MAC_SIZE; i++) {
    if (id1.MAC[i] == id2.MAC[i]) {
      continue;
    }
    return false;
  }

  return true;
}
