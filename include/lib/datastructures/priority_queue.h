#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "lib/datastructures/int_vector.h"

// NOTE: ist es besser, [-1, 0, 1] oder [true, false] zurückzugeben?
// Spätestens für die generische Version kann diese Funktion auch inlined
// werden.
typedef int (*cmp_t)(int, int);

typedef struct {
  cmp_t cmp;
  int_vector_t *items;
} priority_queue_t;

priority_queue_t *priority_queue_create();
unsigned priority_queue_size(priority_queue_t *q);
int priority_queue_peek(priority_queue_t *q);
void priority_queue_push(priority_queue_t *q, int item);
int priority_queue_pop(priority_queue_t *q);
void priority_queue_destroy(priority_queue_t *q);

#endif