#include "lib/datastructures/original/hashmap.h"
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define LOAD_FACTOR_IN_PERCENT 75

MAKE_SPECIFIC_VECTOR_SOURCE(hash_entry_t, hashentry)

void __hm_sanity_check(hashmap_t *hm);
unsigned __hm_next_prime(unsigned start);
void __hm_rehash(hashmap_t *hm);
unsigned __hm_lookup_for_reading(hashmap_t *hm, int key);
unsigned __hm_lookup_for_writing(hashmap_t *hm, int key);

void __hm_sanity_check(hashmap_t *hm) {
  assert(hm != NULL);
  assert(hm->eq != NULL);
  assert(hm->h1 != NULL);
  assert(hm->h2 != NULL);
}

bool __hm_is_prime(unsigned n) {
  if (n < 2)
    return false;

  if (n == 2)
    return true;

  unsigned lim = sqrt(n) + 1;
  for (unsigned i = 2; i < lim; i++) {
    if (n % i == 0)
      return false;
  }

  return true;
}

// TODO: prüfen, ob diese Funktion wirklich funktioniert lol
unsigned __hm_next_prime(unsigned start) {
  for (unsigned c = start; c < UINT_MAX; c++) {
    if (__hm_is_prime(c))
      return c;
  }

  assert(false);
}

void __hm_rehash(hashmap_t *hm) {
  __hm_sanity_check(hm);

  hash_entry_t init = {.state = EMPTY};
  hashentry_vector_t *old = hashentry_vector_clone(hm->entries);

  // Die nächste Größe muss eine Primzahl sein, weil wir dann in den Lookup
  // Funktionen garantieren können, dass jeder Eintrag ein mal besucht wird. Es
  // ist also unmöglich, dort in einer Endlosschleife zu landen.
  unsigned size = hashentry_vector_size(hm->entries);
  unsigned new_size = __hm_next_prime(size + size / 2);

  hashentry_vector_ensure_capacity(hm->entries, new_size);
  hashentry_vector_clear(hm->entries);
  hm->used_count = 0;

  while (!hashentry_vector_full(hm->entries))
    hashentry_vector_append(hm->entries, init);

  for (unsigned i = 0; i < hashentry_vector_size(old); i++) {
    hash_entry_t current = hashentry_vector_at(old, i);

    if (current.state != USED)
      continue;

    hashmap_insert(hm, current.key, current.value);
  }

  hashentry_vector_destroy(old);
}

unsigned __hm_lookup_for_reading(hashmap_t *hm, int key) {
  __hm_sanity_check(hm);

  // NOTE: muss hier % size gemacht werden?
  unsigned size = hashentry_vector_size(hm->entries);
  unsigned initial = hm->h1(key);
  unsigned delta = hm->h2(key);
  if (delta == 0)
    delta = 1;

  for (unsigned x = 0;; x++) {
    unsigned hash = (initial + x * delta) % size;
    hash_entry_t current = hashentry_vector_at(hm->entries, hash);

    if (current.state == DELETED)
      continue;

    if (current.state == USED && hm->eq(current.key, key))
      return hash;
    else
      // Im Moment geben wir einfach einen OOB Index zurück, könnte
      // wahrscheinlich besser gemacht werden. Weil das hier aber interner
      // Library Code ist, ist das nicht so schlimm.
      return size;
  }
}

unsigned __hm_lookup_for_writing(hashmap_t *hm, int key) {
  __hm_sanity_check(hm);

  // Es dürfen nicht zwei Einträge mit gleichen Keys in der HashMap stehen, weil
  // sonst bei einem get() nicht klar ist, welches value zurückgegeben werden
  // soll. Wenn also schon ein Eintrag mit dem gleichen Key existiert, sollte er
  // überschrieben werden.
  unsigned read_index = __hm_lookup_for_reading(hm, key);
  if (read_index < hashentry_vector_size(hm->entries))
    return read_index;

  if (hm->used_count * 100 >=
      hashentry_vector_size(hm->entries) * LOAD_FACTOR_IN_PERCENT)
    __hm_rehash(hm);

  // NOTE: muss hier % size gemacht werden?
  unsigned size = hashentry_vector_size(hm->entries);
  unsigned initial = hm->h1(key);
  unsigned delta = hm->h2(key);
  if (delta == 0)
    delta = 1;

  for (unsigned x = 0;; x++) {
    unsigned hash = (initial + x * delta) % size;

    hash_entry_t current = hashentry_vector_at(hm->entries, hash);
    if (current.state == EMPTY || current.state == DELETED)
      return hash;
  }
}

hashmap_t *hashmap_create(eq_t eq, hash_t h1, hash_t h2) {
  hashmap_t *hm = malloc(sizeof(hashmap_t));
  assert(hm != NULL);

  hm->entries = hashentry_vector_create();
  hm->used_count = 0;
  hm->eq = eq;
  hm->h1 = h1;
  hm->h2 = h2;

  __hm_sanity_check(hm);
  __hm_rehash(hm);

  return hm;
}

void hashmap_insert(hashmap_t *hm, int key, int value) {
  __hm_sanity_check(hm);

  hash_entry_t added = {
      .key = key,
      .value = value,
      .state = USED,
  };

  unsigned index = __hm_lookup_for_writing(hm, key);
  hashentry_vector_insert_at(hm->entries, index, added);
  hm->used_count++;
}

void hashmap_delete(hashmap_t *hm, int key) {
  __hm_sanity_check(hm);

  unsigned index = __hm_lookup_for_reading(hm, key);
  if (index == hashentry_vector_size(hm->entries))
    return;

  hash_entry_t del = {.state = DELETED};
  hashentry_vector_insert_at(hm->entries, index, del);
  hm->used_count--;
}

bool hashmap_exists(hashmap_t *hm, int key) {
  __hm_sanity_check(hm);

  unsigned index = __hm_lookup_for_reading(hm, key);
  return index < hashentry_vector_size(hm->entries);
}

int hashmap_get(hashmap_t *hm, int key) {
  __hm_sanity_check(hm);

  unsigned index = __hm_lookup_for_reading(hm, key);
  assert(index < hashentry_vector_size(hm->entries));

  hash_entry_t entry = hashentry_vector_at(hm->entries, index);
  return entry.value;
}

void hashmap_destroy(hashmap_t *hm) {
  __hm_sanity_check(hm);
  hashentry_vector_destroy(hm->entries);
  free(hm);
}
