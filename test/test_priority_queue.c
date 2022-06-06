#include "lib/datastructures/generic/generic_priority_queue.h"
#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>

// TODO: sollte in eine Trais Datei
int cmp(int a, int b) {
  if (a > b)
    return 1;
  if (a < b)
    return -1;
  return 0;
}

MAKE_SPECIFIC_PRIORITY_QUEUE_HEADER(int, int)
MAKE_SPECIFIC_PRIORITY_QUEUE_SOURCE(int, int, cmp)

Test(priority_queue, create) {
  int_priority_queue_t *q = int_priority_queue_create();
  cr_assert(int_priority_queue_size(q) == 0);
  int_priority_queue_destroy(q);
}

Test(priority_queue, size) {
  int_priority_queue_t *q = int_priority_queue_create();

  int_priority_queue_push(q, 3);
  int_priority_queue_push(q, 5);
  int_priority_queue_push(q, 89);

  cr_assert(int_priority_queue_size(q) == 3);
  int_priority_queue_destroy(q);
}

Test(priority_queue, peek) {
  int_priority_queue_t *q = int_priority_queue_create();

  int_priority_queue_push(q, 3);
  int_priority_queue_push(q, 89);
  int_priority_queue_push(q, 5);
  int_priority_queue_push(q, 1337);
  int_priority_queue_push(q, -7);
  int_priority_queue_push(q, 33);

  cr_assert(int_priority_queue_peek(q) == 1337);
  int_priority_queue_destroy(q);
}

Test(priority_queue, push) {
  int_priority_queue_t *q = int_priority_queue_create();

  for (unsigned i = 0; i < 1000; i++) {
    int_priority_queue_push(q, i);
  }

  int_priority_queue_destroy(q);
}

Test(priority_queue, pop) {
  int_priority_queue_t *q = int_priority_queue_create();

  int_priority_queue_push(q, 3);
  int_priority_queue_push(q, 89);
  int_priority_queue_push(q, 5);
  int_priority_queue_push(q, 1337);
  int_priority_queue_push(q, -7);
  int_priority_queue_push(q, 33);

  cr_assert(int_priority_queue_pop(q) == 1337);
  cr_assert(int_priority_queue_pop(q) == 89);
  cr_assert(int_priority_queue_pop(q) == 33);
  cr_assert(int_priority_queue_pop(q) == 5);
  cr_assert(int_priority_queue_pop(q) == 3);
  cr_assert(int_priority_queue_pop(q) == -7);

  cr_assert(int_priority_queue_size(q) == 0);

  int_priority_queue_destroy(q);
}

Test(priority_queue, pop_invalid, .signal = SIGABRT) {
  int_priority_queue_t *q = int_priority_queue_create();

  int_priority_queue_push(q, 3);
  int_priority_queue_push(q, 89);

  int_priority_queue_pop(q);
  int_priority_queue_pop(q);
  int_priority_queue_pop(q);

  int_priority_queue_destroy(q);
}

Test(priority_queue, destroy) {
  int_priority_queue_t *q = int_priority_queue_create();
  int_priority_queue_destroy(q);
}

Test(priority_queue, destroy_invalid, .signal = SIGABRT) {
  int_priority_queue_t *q = int_priority_queue_create();
  q->items = NULL;
  int_priority_queue_destroy(q);
}