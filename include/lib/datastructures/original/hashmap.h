#ifndef HASHMAP_H
#define HASHMAP_H

#include "lib/datastructures/generic/generic_vector.h"

typedef bool (*eq_t)(int, int);
typedef unsigned (*hash_t)(int);

typedef enum { DELETED, EMPTY, USED } hash_state_t;

typedef struct {
  int key;
  int value;
  hash_state_t state;
} hash_entry_t;

#ifndef HASHENTRY_VECTOR_H
#define HASHENTRY_VECTOR_H

MAKE_SPECIFIC_VECTOR_HEADER(hash_entry_t, hashentry)

#endif

// deleted count muss gespeichert werden
// rehash bei Vergrößerung der Map

// hash Funktion für generischen Typ muss bei Macro-Aufruf übergeben werden,
// danach kann diese Funktion aufgerufen werden.
// eq Funktion sollte auch übergeben werden

typedef struct {
  hashentry_vector_t *entries;
  eq_t eq;
  hash_t h1;
  hash_t h2;
  unsigned used_count;
} hashmap_t;

hashmap_t *hashmap_create(eq_t eq, hash_t h1, hash_t h2);
void hashmap_insert(hashmap_t *hm, int key, int value);
void hashmap_delete(hashmap_t *hm, int key);
bool hashmap_exists(hashmap_t *hm, int key);
int hashmap_get(hashmap_t *hm, int key);
void hashmap_destroy(hashmap_t *hm);

#endif