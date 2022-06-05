#include "lib/datastructures/int_vector.h"
#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>

Test(vector, create) {
  int_vector_t *v = int_vector_create();
  cr_assert(int_vector_size(v) == 0);
  int_vector_destroy(v);
}

Test(vector, create_with_capacity) {
  int_vector_t *v = int_vector_create_with_capacity(10);
  cr_assert(int_vector_size(v) == 0);
  // NOTE: Funktion dafür schreiben? Ich bin mir aber nicht sicher, ob das
  // irgendjemand außer den Tests braucht.
  cr_assert(v->capacity == 10);
  int_vector_destroy(v);
}

Test(vector, access) {
  int_vector_t *v = int_vector_create();
  int_vector_append(v, 42);
  cr_assert(int_vector_at(v, 0) == 42);
  int_vector_destroy(v);
}

Test(vector, access_oob, .signal = SIGABRT) {
  int_vector_t *v = int_vector_create();
  int_vector_at(v, 3);
  int_vector_destroy(v);
}

Test(vector, insert) {
  int items[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  int_vector_t *v = int_vector_create();

  for (unsigned i = 0; i < 10; i++) {
    int_vector_append(v, items[i]);
  }

  for (unsigned i = 0; i < 10; i++) {
    cr_assert(int_vector_at(v, i) == items[i]);
  }

  int_vector_destroy(v);
}

Test(vector, swap) {
  int items[5] = {3, 87, 2, 25, 48};
  int_vector_t *v = int_vector_create_with_capacity(5);

  for (unsigned i = 0; i < 5; i++) {
    int_vector_append(v, items[i]);
  }

  int_vector_swap(v, 1, 4);
  int_vector_swap(v, 2, 3);

  int items_after[5] = {3, 48, 25, 2, 87};

  for (unsigned i = 0; i < 5; i++) {
    cr_assert(int_vector_at(v, i) == items_after[i]);
  }

  int_vector_destroy(v);
}

Test(vector, full) {
  int items[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  int_vector_t *v = int_vector_create_with_capacity(10);

  for (unsigned i = 0; i < 10; i++) {
    int_vector_append(v, items[i]);
  }

  cr_assert(int_vector_full(v));
  int_vector_destroy(v);
}

Test(vector, remove) {
  int items[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  int_vector_t *v = int_vector_create();

  for (unsigned i = 0; i < 10; i++) {
    int_vector_append(v, items[i]);
  }

  cr_assert(int_vector_remove(v, 7) == items[7]);
  cr_assert(int_vector_remove(v, 3) == items[3]);
  cr_assert(int_vector_remove(v, 0) == items[0]);
  cr_assert(int_vector_size(v) == 7);

  int items_after[7] = {9, 1, 2, 8, 4, 5, 6};

  for (unsigned i = 0; i < 7; i++) {
    cr_assert(int_vector_at(v, i) == items_after[i]);
  }

  int_vector_destroy(v);
}

Test(vector, remove_oob, .signal = SIGABRT) {
  int_vector_t *v = int_vector_create();
  int_vector_remove(v, 13);
  int_vector_destroy(v);
}

Test(vector, destroy) {
  int_vector_t *v = int_vector_create();
  int_vector_destroy(v);
}

Test(vector, destroy_invalid, .signal = SIGABRT) {
  int_vector_t *v = int_vector_create();
  v->size = 9001;
  int_vector_destroy(v);
}