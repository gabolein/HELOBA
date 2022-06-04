#include "lib/datastructures/priority_queue.h"
#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>

Test(priority_queue, create) {
  priority_queue_t *q = priority_queue_create();
  cr_assert(priority_queue_size(q) == 0);
  priority_queue_destroy(q);
}

Test(priority_queue, size) {
  priority_queue_t *q = priority_queue_create();

  priority_queue_push(q, 3);
  priority_queue_push(q, 5);
  priority_queue_push(q, 89);

  cr_assert(priority_queue_size(q) == 3);
  priority_queue_destroy(q);
}

Test(priority_queue, peek) {
  priority_queue_t *q = priority_queue_create();

  priority_queue_push(q, 3);
  priority_queue_push(q, 89);
  priority_queue_push(q, 5);
  priority_queue_push(q, 1337);
  priority_queue_push(q, -7);
  priority_queue_push(q, 33);

  cr_assert(priority_queue_peek(q) == 1337);
  priority_queue_destroy(q);
}

Test(priority_queue, push) {
  priority_queue_t *q = priority_queue_create();

  for (unsigned i = 0; i < 1000; i++) {
    priority_queue_push(q, i);
  }

  priority_queue_destroy(q);
}

Test(priority_queue, pop) {
  priority_queue_t *q = priority_queue_create();

  priority_queue_push(q, 3);
  priority_queue_push(q, 89);
  priority_queue_push(q, 5);
  priority_queue_push(q, 1337);
  priority_queue_push(q, -7);
  priority_queue_push(q, 33);

  cr_assert(priority_queue_pop(q) == 1337);
  cr_assert(priority_queue_pop(q) == 89);
  cr_assert(priority_queue_pop(q) == 33);
  cr_assert(priority_queue_pop(q) == 5);
  cr_assert(priority_queue_pop(q) == 3);
  cr_assert(priority_queue_pop(q) == -7);

  cr_assert(priority_queue_size(q) == 0);

  priority_queue_destroy(q);
}

Test(priority_queue, pop_invalid, .signal = SIGABRT) {
  priority_queue_t *q = priority_queue_create();

  priority_queue_push(q, 3);
  priority_queue_push(q, 89);

  priority_queue_pop(q);
  priority_queue_pop(q);
  priority_queue_pop(q);

  priority_queue_destroy(q);
}

Test(priority_queue, destroy) {
  priority_queue_t *q = priority_queue_create();
  priority_queue_destroy(q);
}

Test(priority_queue, destroy_invalid, .signal = SIGABRT) {
  priority_queue_t *q = priority_queue_create();
  q->cmp = NULL;
  priority_queue_destroy(q);
}