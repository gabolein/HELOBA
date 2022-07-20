#include "src/protocol/tree.h"
#include "src/config.h"
#include "src/protocol/message.h"
#include <assert.h>

// Returns whether f is inside the frequency
// band [FREQUENCY_BASE, FREQUENCY_CEILING]
bool is_valid_tree_node(frequency_t f) {
  return f >= FREQUENCY_BASE && f <= FREQUENCY_CEILING;
}

// Returns the parent frequency, if it lies inside our frequency bounds,
// or f itself.
frequency_t tree_node_parent(frequency_t f) {
  // NOTE: Ist Underflow, wenn f = FREQUENCY_BASE, schlimm?
  frequency_t parent = FREQUENCY_BASE + (f - FREQUENCY_BASE - 1) / 2;
  return is_valid_tree_node(parent) ? parent : f;
}

// Returns the lhs frequency, if it lies inside our frequency bounds,
// or f itself.
frequency_t tree_node_lhs(frequency_t f) {
  frequency_t lhs = FREQUENCY_BASE + (f - FREQUENCY_BASE) * 2 + 1;
  return is_valid_tree_node(lhs) ? lhs : f;
}

// Returns the rhs frequency, if it lies inside our frequency bounds,
// or f itself.
frequency_t tree_node_rhs(frequency_t f) {
  frequency_t rhs = FREQUENCY_BASE + (f - FREQUENCY_BASE) * 2 + 2;
  return is_valid_tree_node(rhs) ? rhs : f;
}

unsigned tree_node_height(frequency_t f) {
  unsigned height = 0;
  while (f != tree_node_parent(f)) {
    height++;
    f = tree_node_parent(f);
  }

  return height;
}

// Returns whether a lies higher in the tree than b.
bool tree_node_gt(frequency_t a, frequency_t b) {
  return tree_node_height(a) < tree_node_height(b);
}