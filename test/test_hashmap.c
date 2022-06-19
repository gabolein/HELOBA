#include "lib/datastructures/generic/generic_hashmap.h"
#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>

// TODO: sollte in eine Traits Datei
bool eq(int a, int b) { return a == b; }

MAKE_SPECIFIC_HASHMAP_HEADER(int, int, ii);
MAKE_SPECIFIC_HASHMAP_SOURCE(int, int, ii, eq)

Test(hashmap, create) {
  ii_hashmap_t *hm = ii_hashmap_create();
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

Test(hashmap, delete) {
  ii_hashmap_t *hm = ii_hashmap_create();

  ii_hashmap_insert(hm, 13, 42);
  ii_hashmap_delete(hm, 13);

  ii_hashmap_destroy(hm);
}

Test(hashmap, exists) {
  ii_hashmap_t *hm = ii_hashmap_create();

  ii_hashmap_insert(hm, 13, 42);
  ii_hashmap_insert(hm, 74853875, -15);
  cr_assert(ii_hashmap_exists(hm, 74853875));

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

Test(hashmap, destroy) {
  ii_hashmap_t *hm = ii_hashmap_create();
  ii_hashmap_destroy(hm);
}
