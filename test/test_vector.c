#include "lib/datastructures/vector.h"
#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>

Test(vector, create) {
  vector_t *v = vector_create();
  cr_assert(vector_size(v) == 0);
  vector_destroy(v);
}

Test(vector, create_with_capacity) {
  vector_t *v = vector_create_with_capacity(10);
  cr_assert(vector_size(v) == 0);
  // NOTE: Funktion dafür schreiben? Ich bin mir aber nicht sicher, ob das
  // irgendjemand außer den Tests braucht.
  cr_assert(v->capacity == 10);
  vector_destroy(v);
}

Test(vector, access) {
  vector_t *v = vector_create();
  vector_append(v, 42);
  cr_assert(vector_at(v, 0) == 42);
  vector_destroy(v);
}

Test(vector, access_oob, .signal = SIGABRT) {
  vector_t *v = vector_create();
  vector_at(v, 3);
  vector_destroy(v);
}

Test(vector, insert) {
  int items[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  vector_t *v = vector_create();

  for (unsigned i = 0; i < 10; i++) {
    vector_append(v, items[i]);
  }

  for (unsigned i = 0; i < 10; i++) {
    cr_assert(vector_at(v, i) == items[i]);
  }

  vector_destroy(v);
}

Test(vector, swap) {
  int items[5] = {3, 87, 2, 25, 48};
  vector_t *v = vector_create_with_capacity(5);

  for (unsigned i = 0; i < 5; i++) {
    vector_append(v, items[i]);
  }

  vector_swap(v, 1, 4);
  vector_swap(v, 2, 3);

  int items_after[5] = {3, 48, 25, 2, 87};

  for (unsigned i = 0; i < 5; i++) {
    cr_assert(vector_at(v, i) == items_after[i]);
  }

  vector_destroy(v);
}

Test(vector, remove) {
  int items[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  vector_t *v = vector_create();

  for (unsigned i = 0; i < 10; i++) {
    vector_append(v, items[i]);
  }

  cr_assert(vector_remove(v, 7) == items[7]);
  cr_assert(vector_remove(v, 3) == items[3]);
  cr_assert(vector_remove(v, 0) == items[0]);
  cr_assert(vector_size(v) == 7);

  int items_after[7] = {9, 1, 2, 8, 4, 5, 6};

  for (unsigned i = 0; i < 7; i++) {
    cr_assert(vector_at(v, i) == items_after[i]);
  }

  vector_destroy(v);
}

Test(vector, remove_oob, .signal = SIGABRT) {
  vector_t *v = vector_create();
  vector_remove(v, 13);
  vector_destroy(v);
}

Test(vector, destroy) {
  vector_t *v = vector_create();
  vector_destroy(v);
}

Test(vector, destroy_invalid, .signal = SIGABRT) {
  vector_t *v = vector_create();
  v->size = 9001;
  vector_destroy(v);
}