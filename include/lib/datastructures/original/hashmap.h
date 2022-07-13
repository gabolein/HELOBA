#ifndef HASHMAP_H
#define HASHMAP_H

#include "lib/datastructures/generic/generic_vector.h"
#include "lib/datastructures/original/priority_queue.h"

typedef bool (*eq_t)(int, int);

typedef enum { DELETED, EMPTY, USED } hash_state_t;

typedef struct {
  int key;
  int value;
  hash_state_t state;
} hash_entry_t;

MAKE_SPECIFIC_VECTOR_HEADER(hash_entry_t, hashentry)

typedef struct {
  hashentry_vector_t *entries;
  eq_t eq;
  unsigned used_count;
} hashmap_t;

hashmap_t *hashmap_create(eq_t eq);
unsigned hashmap_size(hashmap_t *hm);
void hashmap_insert(hashmap_t *hm, int key, int value);
void hashmap_remove(hashmap_t *hm, int key);
bool hashmap_exists(hashmap_t *hm, int key);
int_vector_t *hashmap_keys(hashmap_t *hm);
int hashmap_get(hashmap_t *hm, int key);
void hashmap_clear(hashmap_t *hm);
void hashmap_destroy(hashmap_t *hm);

#endif