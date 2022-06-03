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
  int item;
  vector_t *v = vector_create();
  cr_assert(vector_at(v, 0, &item) == false);
  cr_assert(vector_insert(v, 42) == true);
  cr_assert(vector_at(v, 0, &item) == true);
  cr_assert(item == 42);
  vector_destroy(v);
}

Test(vector, insert) {
  int items[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  vector_t *v = vector_create();

  for (unsigned i = 0; i < 10; i++) {
    vector_insert(v, items[i]);
  }

  for (unsigned i = 0; i < 10; i++) {
    int item;
    cr_assert(vector_at(v, i, &item) == true);
    cr_assert(item == items[i]);
  }

  vector_destroy(v);
}

Test(vector, remove) {
  int items[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  vector_t *v = vector_create();

  for (unsigned i = 0; i < 10; i++) {
    vector_insert(v, items[i]);
  }

  cr_assert(vector_remove(v, 7) == true);
  cr_assert(vector_remove(v, 3) == true);
  cr_assert(vector_remove(v, 0) == true);
  cr_assert(vector_remove(v, 9) == false);
  cr_assert(vector_size(v) == 7);

  int items_after[7] = {1, 2, 4, 5, 6, 8, 9};

  for (unsigned i = 0; i < 7; i++) {
    int item;
    cr_assert(vector_at(v, i, &item) == true);
    cr_assert(item == items_after[i]);
  }

  vector_destroy(v);
}

Test(vector, destroy) {
  vector_t *v = vector_create();
  cr_assert(vector_destroy(v) == true);

  vector_t *w = vector_create();
  w->size = 9001;
  cr_assert(vector_destroy(w) == false);
  w->size = 0;
  cr_assert(vector_destroy(w) == true);
}