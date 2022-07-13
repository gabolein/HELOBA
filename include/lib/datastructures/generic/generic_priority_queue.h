#ifndef GENERIC_PRIORITY_QUEUE_H
#define GENERIC_PRIORITY_QUEUE_H

#include "lib/datastructures/generic/generic_vector.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

unsigned __gpq_lhs(unsigned i);
unsigned __gpq_rhs(unsigned i);
unsigned __gpq_parent(unsigned i);

// Es langt aus, nur den Vektor Header neu zu generieren, weil der Vector Code
// einfach vom Linker gefunden werden kann.
#define MAKE_SPECIFIC_PRIORITY_QUEUE_HEADER(T, name)                           \
  MAKE_SPECIFIC_VECTOR_HEADER(T, name)                                         \
                                                                               \
  typedef struct {                                                             \
    name##_vector_t *items;                                                    \
  } name##_priority_queue_t;                                                   \
                                                                               \
  name##_priority_queue_t *name##_priority_queue_create();                     \
  unsigned name##_priority_queue_size(name##_priority_queue_t *q);             \
  bool name##_priority_queue_empty(name##_priority_queue_t *q);                \
  T name##_priority_queue_peek(name##_priority_queue_t *q);                    \
  void name##_priority_queue_push(name##_priority_queue_t *q, T item);         \
  T name##_priority_queue_pop(name##_priority_queue_t *q);                     \
  void name##_priority_queue_clear(name##_priority_queue_t *q);                \
  void name##_priority_queue_destroy(name##_priority_queue_t *q);

#define MAKE_SPECIFIC_PRIORITY_QUEUE_SOURCE(T, name, cmp)                      \
  void __##name##_pq_sanity_check(name##_priority_queue_t *q);                 \
  void __##name##_pq_heapify(name##_priority_queue_t *q, unsigned root_index); \
                                                                               \
  void __##name##_pq_sanity_check(name##_priority_queue_t *q) {                \
    assert(q != NULL);                                                         \
    assert(cmp != NULL);                                                       \
  }                                                                            \
                                                                               \
  void __##name##_pq_heapify(name##_priority_queue_t *q,                       \
                             unsigned root_index) {                            \
    __##name##_pq_sanity_check(q);                                             \
                                                                               \
    unsigned largest_index = root_index;                                       \
                                                                               \
    if (__gpq_lhs(root_index) < name##_vector_size(q->items)) {                \
      T root = name##_vector_at(q->items, root_index);                         \
      T lhs = name##_vector_at(q->items, __gpq_lhs(root_index));               \
      largest_index =                                                          \
          cmp(root, lhs) == 1 ? root_index : __gpq_lhs(root_index);            \
    }                                                                          \
                                                                               \
    if (__gpq_rhs(root_index) < name##_vector_size(q->items)) {                \
      T largest = name##_vector_at(q->items, largest_index);                   \
      T rhs = name##_vector_at(q->items, __gpq_rhs(root_index));               \
      largest_index =                                                          \
          cmp(largest, rhs) == 1 ? largest_index : __gpq_rhs(root_index);      \
    }                                                                          \
                                                                               \
    if (largest_index != root_index) {                                         \
      name##_vector_swap(q->items, root_index, largest_index);                 \
      __##name##_pq_heapify(q, largest_index);                                 \
    }                                                                          \
  }                                                                            \
                                                                               \
  name##_priority_queue_t *name##_priority_queue_create() {                    \
    name##_priority_queue_t *q = malloc(sizeof(name##_priority_queue_t));      \
    assert(q != NULL);                                                         \
                                                                               \
    q->items = name##_vector_create();                                         \
    assert(q != NULL);                                                         \
                                                                               \
    return q;                                                                  \
  }                                                                            \
                                                                               \
  unsigned name##_priority_queue_size(name##_priority_queue_t *q) {            \
    __##name##_pq_sanity_check(q);                                             \
    return name##_vector_size(q->items);                                       \
  }                                                                            \
                                                                               \
  bool name##_priority_queue_empty(name##_priority_queue_t *q) {               \
    __##name##_pq_sanity_check(q);                                             \
    return name##_vector_size(q->items) == 0;                                  \
  }                                                                            \
                                                                               \
  T name##_priority_queue_peek(name##_priority_queue_t *q) {                   \
    __##name##_pq_sanity_check(q);                                             \
    return name##_vector_at(q->items, 0);                                      \
  }                                                                            \
                                                                               \
  void name##_priority_queue_push(name##_priority_queue_t *q, T item) {        \
    __##name##_pq_sanity_check(q);                                             \
                                                                               \
    name##_vector_append(q->items, item);                                      \
                                                                               \
    for (unsigned i = name##_vector_size(q->items) - 1; i > 0;                 \
         i = __gpq_parent(i)) {                                                \
      T parent = name##_vector_at(q->items, __gpq_parent(i));                  \
      T current = name##_vector_at(q->items, i);                               \
                                                                               \
      if (cmp(parent, current) == 1)                                           \
        return;                                                                \
                                                                               \
      name##_vector_swap(q->items, __gpq_parent(i), i);                        \
    }                                                                          \
  }                                                                            \
                                                                               \
  T name##_priority_queue_pop(name##_priority_queue_t *q) {                    \
    __##name##_pq_sanity_check(q);                                             \
                                                                               \
    T removed = name##_vector_remove(q->items, 0);                             \
    __##name##_pq_heapify(q, 0);                                               \
    return removed;                                                            \
  }                                                                            \
                                                                               \
  void name##_priority_queue_clear(name##_priority_queue_t *q) {               \
    __##name##_pq_sanity_check(q);                                             \
    name##_vector_clear(q->items);                                             \
  }                                                                            \
                                                                               \
  void name##_priority_queue_destroy(name##_priority_queue_t *q) {             \
    __##name##_pq_sanity_check(q);                                             \
                                                                               \
    name##_vector_destroy(q->items);                                           \
    free(q);                                                                   \
  }

#endif