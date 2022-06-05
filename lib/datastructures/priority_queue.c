#include "lib/datastructures/priority_queue.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_INIT_CAPACITY 8

#define LHS(x) (2 * (x) + 1)
#define RHS(x) (2 * (x) + 2)
#define PARENT(x) (((x)-1) / 2)

int __pq_default_cmp(int a, int b);
void __pq_sanity_check(priority_queue_t *q);
void __pq_heapify(priority_queue_t *q, unsigned root_index);

int __pq_default_cmp(int a, int b) {
  if (a < b)
    return -1;
  if (a > b)
    return 1;
  return 0;
}

void __pq_sanity_check(priority_queue_t *q) {
  assert(q != NULL);
  assert(q->cmp != NULL);
}

void __pq_heapify(priority_queue_t *q, unsigned root_index) {
  __pq_sanity_check(q);

  unsigned largest_index = root_index;

  if (LHS(root_index) < int_vector_size(q->items)) {
    int root = int_vector_at(q->items, root_index);
    int lhs = int_vector_at(q->items, LHS(root_index));
    largest_index = q->cmp(root, lhs) == 1 ? root_index : LHS(root_index);
  }

  if (RHS(root_index) < int_vector_size(q->items)) {
    int largest = int_vector_at(q->items, largest_index);
    int rhs = int_vector_at(q->items, RHS(root_index));
    largest_index = q->cmp(largest, rhs) == 1 ? largest_index : RHS(root_index);
  }

  if (largest_index != root_index) {
    int_vector_swap(q->items, root_index, largest_index);
    __pq_heapify(q, largest_index);
  }
}

priority_queue_t *priority_queue_create() {
  priority_queue_t *q = malloc(sizeof(priority_queue_t));
  assert(q != NULL);

  q->cmp = __pq_default_cmp;
  q->items = int_vector_create_with_capacity(DEFAULT_INIT_CAPACITY);
  assert(q != NULL);

  return q;
}

void priority_queue_set_comparator(priority_queue_t *q, cmp_t cmp) {
  __pq_sanity_check(q);
  q->cmp = cmp;
}

unsigned priority_queue_size(priority_queue_t *q) {
  return int_vector_size(q->items);
}

int priority_queue_peek(priority_queue_t *q) {
  __pq_sanity_check(q);
  return int_vector_at(q->items, 0);
}

void priority_queue_push(priority_queue_t *q, int item) {
  __pq_sanity_check(q);

  int_vector_append(q->items, item);

  for (unsigned i = int_vector_size(q->items) - 1; i > 0; i = PARENT(i)) {
    int parent = int_vector_at(q->items, PARENT(i));
    int current = int_vector_at(q->items, i);

    if (q->cmp(parent, current) == 1)
      return;

    int_vector_swap(q->items, PARENT(i), i);
  }
}

int priority_queue_pop(priority_queue_t *q) {
  __pq_sanity_check(q);

  int removed = int_vector_remove(q->items, 0);
  __pq_heapify(q, 0);
  return removed;
}

void priority_queue_destroy(priority_queue_t *q) {
  __pq_sanity_check(q);

  int_vector_destroy(q->items);
  free(q);
}