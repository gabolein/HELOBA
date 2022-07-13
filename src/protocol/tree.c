#include "src/protocol/tree.h"
#include "src/protocol/message.h"
#include <assert.h>

#define FREQUENCY_BASE 800
#define FREQUENCY_CEILING 950

// Returns whether f is inside the frequency
// band [FREQUENCY_BASE, FREQUENCY_CEILING]
bool is_valid_tree_node(frequency_t f) {
  return f >= FREQUENCY_BASE && f <= FREQUENCY_CEILING;
}

// Returns the parent frequency, if it lies inside our frequency bounds,
// or f itself.
frequency_t tree_node_parent(frequency_t f) {
  if (!is_valid_tree_node(f)) {
    return f;
  }

  return FREQUENCY_BASE + (f - FREQUENCY_BASE) / 2;
}

// Returns the lhs frequency, if it lies inside our frequency bounds,
// or f itself.
frequency_t tree_node_lhs(frequency_t f) {
  if (!is_valid_tree_node(f)) {
    return f;
  }

  frequency_t lhs = FREQUENCY_BASE + (f - FREQUENCY_BASE) * 2 + 1;
  return lhs > FREQUENCY_CEILING ? f : lhs;
}

// Returns the rhs frequency, if it lies inside our frequency bounds,
// or f itself.
frequency_t tree_node_rhs(frequency_t f) {
  if (!is_valid_tree_node(f)) {
    return f;
  }

  frequency_t rhs = FREQUENCY_BASE + (f - FREQUENCY_BASE) * 2 + 1;
  return rhs > FREQUENCY_CEILING ? f : rhs;
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