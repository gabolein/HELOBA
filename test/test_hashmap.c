#include "lib/datastructures/hashmap.h"
#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>

bool eq(int a, int b) { return a == b; }

unsigned h1(int key) {
  key = ((key >> 16) ^ key) * 0x45d9f3b;
  key = ((key >> 16) ^ key) * 0x45d9f3b;
  key = (key >> 16) ^ key;
  return key;
}

unsigned h2(int key) {
  key += ~(key << 15);
  key ^= (key >> 10);
  key += (key << 3);
  key ^= (key >> 6);
  key += ~(key << 11);
  key ^= (key >> 16);
  return key;
}

Test(hashmap, create) {
  hashmap_t *hm = hashmap_create(eq, h1, h2);
  hashmap_destroy(hm);
}

Test(hashmap, insert) {
  hashmap_t *hm = hashmap_create(eq, h1, h2);

  for (unsigned i = 0; i < 1000; i++) {
    hashmap_insert(hm, i, i + 20);
  }

  hashmap_destroy(hm);
}

Test(hashmap, insert_same_key) {
  hashmap_t *hm = hashmap_create(eq, h1, h2);

  hashmap_insert(hm, 13, 42);
  hashmap_insert(hm, 13, 37);

  cr_assert(hashmap_get(hm, 13) == 37);

  hashmap_destroy(hm);
}

Test(hashmap, delete) {
  hashmap_t *hm = hashmap_create(eq, h1, h2);

  hashmap_insert(hm, 13, 42);
  hashmap_delete(hm, 13);

  hashmap_destroy(hm);
}

Test(hashmap, exists) {
  hashmap_t *hm = hashmap_create(eq, h1, h2);

  hashmap_insert(hm, 13, 42);
  hashmap_insert(hm, 74853875, -15);
  cr_assert(hashmap_exists(hm, 74853875));

  hashmap_destroy(hm);
}

Test(hashmap, get) {
  hashmap_t *hm = hashmap_create(eq, h1, h2);

  hashmap_insert(hm, 13, 42);
  cr_assert(hashmap_get(hm, 13) == 42);

  hashmap_destroy(hm);
}

Test(hashmap, get_invalid, .signal = SIGABRT) {
  hashmap_t *hm = hashmap_create(eq, h1, h2);

  hashmap_get(hm, 4200);

  hashmap_destroy(hm);
}

Test(hashmap, destroy) {
  hashmap_t *hm = hashmap_create(eq, h1, h2);
  hashmap_destroy(hm);
}
