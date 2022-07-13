#ifndef TREE_H
#define TREE_H

// FIXME: braucht eigentlich nur die Deklaration von frequency_t, irgendwie
// aufteilen?
#include "src/protocol/message.h"

bool is_valid_tree_node(frequency_t f);
frequency_t tree_node_parent(frequency_t f);
frequency_t tree_node_lhs(frequency_t f);
frequency_t tree_node_rhs(frequency_t f);
bool tree_node_gt(frequency_t a, frequency_t b);

#endif