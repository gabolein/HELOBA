#include "lib/datastructures/generic/generic_hashmap.h"
#include "lib/datastructures/generic/generic_vector.h"
#include <criterion/criterion.h>
#include <criterion/internal/test.h>
#include <signal.h>
#include <stdio.h>

// TODO: sollte in eine Traits Datei
bool eq(int a, int b) { return a == b; }

MAKE_SPECIFIC_HASHMAP_HEADER(int, int, ii)
MAKE_SPECIFIC_HASHMAP_SOURCE(int, int, ii, eq)

Test(hashmap, create) {
  ii_hashmap_t *hm = ii_hashmap_create();
  ii_hashmap_destroy(hm);
}

Test(hashmap, size) {
  ii_hashmap_t *hm = ii_hashmap_create();
  cr_assert(ii_hashmap_size(hm) == 0);

  ii_hashmap_insert(hm, 13, 37);
  cr_assert(ii_hashmap_size(hm) == 1);

  ii_hashmap_remove(hm, 13);
  cr_assert(ii_hashmap_size(hm) == 0);

  ii_hashmap_destroy(hm);
}

Test(hashmap, insert) {
  ii_hashmap_t *hm = ii_hashmap_create();

  for (unsigned i = 0; i < 1000; i++) {
    ii_hashmap_insert(hm, i, i + 20);
  }

  ii_hashmap_destroy(hm);
}

Test(hashmap, insert_same_key) {
  ii_hashmap_t *hm = ii_hashmap_create();

  ii_hashmap_insert(hm, 13, 42);
  ii_hashmap_insert(hm, 13, 37);

  cr_assert(ii_hashmap_get(hm, 13) == 37);

  ii_hashmap_destroy(hm);
}

Test(hashmap, remove) {
  ii_hashmap_t *hm = ii_hashmap_create();

  ii_hashmap_insert(hm, 13, 42);
  ii_hashmap_remove(hm, 13);

  ii_hashmap_destroy(hm);
}

Test(hashmap, exists) {
  ii_hashmap_t *hm = ii_hashmap_create();

  ii_hashmap_insert(hm, 13, 42);
  ii_hashmap_insert(hm, 74853875, -15);
  cr_assert(ii_hashmap_exists(hm, 74853875));

  ii_hashmap_destroy(hm);
}

Test(hashmap, keys) {
  ii_hashmap_t *hm = ii_hashmap_create();

  for (unsigned i = 0; i < 10; i++) {
    ii_hashmap_insert(hm, i, i + 20);
  }

  int_vector_t *keys = ii_hashmap_keys(hm);
  cr_assert(int_vector_size(keys) == 10);

  // NOTE: Keys werden nicht in der Reihenfolge zurückgegeben, in der sie
  // eingefügt wurden. Deswegen müssen wir diese inverse Prüfung machen
  for (unsigned i = 0; i < int_vector_size(keys); i++) {
    cr_assert(ii_hashmap_exists(hm, int_vector_at(keys, i)));
  }

  // NOTE: keys wurde innerhalb von ii_hashmap_keys() neu erstellt, wir müssen
  // den Vector wieder freigeben
  int_vector_destroy(keys);
  ii_hashmap_destroy(hm);
}

Test(hashmap, get) {
  ii_hashmap_t *hm = ii_hashmap_create();

  ii_hashmap_insert(hm, 13, 42);
  cr_assert(ii_hashmap_get(hm, 13) == 42);

  ii_hashmap_destroy(hm);
}

Test(hashmap, get_invalid, .signal = SIGABRT) {
  ii_hashmap_t *hm = ii_hashmap_create();

  ii_hashmap_get(hm, 4200);

  ii_hashmap_destroy(hm);
}

Test(hashmap, clear) {
  ii_hashmap_t *hm = ii_hashmap_create();

  for (unsigned i = 0; i < 10; i++) {
    ii_hashmap_insert(hm, i, i + 20);
  }

  ii_hashmap_clear(hm);

  cr_assert(ii_hashmap_size(hm) == 0);
  for (unsigned i = 0; i < 10; i++) {
    cr_assert(!ii_hashmap_exists(hm, i));
  }

  ii_hashmap_destroy(hm);
}

Test(hashmap, destroy) {
  ii_hashmap_t *hm = ii_hashmap_create();
  ii_hashmap_destroy(hm);
}
