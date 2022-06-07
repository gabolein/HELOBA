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

Test(vector, at) {
  int_vector_t *v = int_vector_create();
  int_vector_append(v, 42);
  cr_assert(int_vector_at(v, 0) == 42);
  int_vector_destroy(v);
}

Test(vector, at_invalid, .signal = SIGABRT) {
  int_vector_t *v = int_vector_create();
  int_vector_at(v, 3);
  int_vector_destroy(v);
}

Test(vector, insert_at) {
  int items[5] = {3, 87, 2, 25, 48};
  int_vector_t *v = int_vector_create_with_capacity(5);

  for (unsigned i = 0; i < 5; i++) {
    int_vector_append(v, items[i]);
  }

  int_vector_insert_at(v, 3, 60);
  int_vector_insert_at(v, 1, 10);

  int items_after[5] = {3, 10, 2, 60, 48};

  for (unsigned i = 0; i < 5; i++) {
    cr_assert(int_vector_at(v, i) == items_after[i]);
  }

  int_vector_destroy(v);
}

Test(vector, append) {
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

Test(vector, remove_invalid, .signal = SIGABRT) {
  int_vector_t *v = int_vector_create();
  int_vector_remove(v, 13);
  int_vector_destroy(v);
}

Test(vector, clear) {
  int_vector_t *v = int_vector_create();
  int_vector_append(v, 33);
  int_vector_clear(v);
  cr_assert(int_vector_size(v) == 0);
}

Test(vector, clone) {
  int items[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  int_vector_t *v = int_vector_create();

  for (unsigned i = 0; i < 10; i++) {
    int_vector_append(v, items[i]);
  }

  int_vector_t *cloned = int_vector_clone(v);

  cr_assert(v->capacity == cloned->capacity);
  cr_assert(v->size == cloned->size);

  for (unsigned i = 0; i < int_vector_size(v); i++) {
    cr_assert(int_vector_at(v, i) == int_vector_at(cloned, i));
  }
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

typedef struct test_struct{
  int a;
  void* b;
  char d;
} test_struct_vector;
int cmp_test_struct(test_struct_vector struct1, test_struct_vector struct2) {
  if (struct1.a > struct2.a)
    return 1;
  if (struct1.a < struct2.a)
    return -1;
  return 0;
}

MAKE_SPECIFIC_VECTOR_HEADER(test_struct_vector, test_struct_vector)
MAKE_SPECIFIC_VECTOR_SOURCE(test_struct_vector, test_struct_vector)

Test(vector, generic_create) {
  test_struct_vector_vector_t *v = test_struct_vector_vector_create();
  cr_assert(test_struct_vector_vector_size(v) == 0);
  test_struct_vector_vector_destroy(v);
}
