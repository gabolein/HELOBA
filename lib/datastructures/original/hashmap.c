#include "lib/datastructures/original/hashmap.h"
#include "lib/datastructures/original/priority_queue.h"
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define LOAD_FACTOR_IN_PERCENT 75
#define __HM_SEED1 0x378b0322
#define __HM_SEED2 0x9fa770ba

MAKE_SPECIFIC_VECTOR_SOURCE(hash_entry_t, hashentry)

uint32_t __hm_murmur3(const void *key, int len, uint32_t seed);
void __hm_sanity_check(hashmap_t *hm);
unsigned __hm_next_prime(unsigned start);
void __hm_rehash(hashmap_t *hm);
unsigned __hm_lookup_for_reading(hashmap_t *hm, int key);
unsigned __hm_lookup_for_writing(hashmap_t *hm, int key);

// Murmur3 Implementierung kopiert von:
// https://github.com/abrandoned/murmur3/blob/master/MurmurHash3.c

static inline uint32_t rotl32(uint32_t x, int8_t r) {
  return (x << r) | (x >> (32 - r));
}

static inline uint32_t getblock(const uint32_t *p, int i) { return p[i]; }

static inline void bmix(uint32_t *h1, uint32_t *k1, uint32_t *c1,
                        uint32_t *c2) {
  *k1 *= *c1;
  *k1 = rotl32(*k1, 15);
  *k1 *= *c2;
  *h1 ^= *k1;
  *h1 = rotl32(*h1, 13);
  *h1 = *h1 * 5 + 0xe6546b64;
}

static inline uint32_t fmix(uint32_t h) {
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}

// FIXME: optimierte Version für 64bit Architektur benutzen
// NOTE: ist es ok, den Typ von len zu unsigned zu ändern?
// NOTE: Weil diese Hashfunktion mit dem unterliegenden Speicher des jeweiligen
// Datentyps arbeitet, muss man bei structs extrem gut aufpassen, dass vor dem
// Benutzen der HashMap auf allen Items memset() bei der Erstellung aufgerufen
// wurde. Ansonsten kann es sein, dass Paddingbytes unterschiedliche Werte haben
// und die HashMap in dem Fall unerwartete Ergebnisse liefert!
uint32_t __hm_murmur3(const void *key, int len, uint32_t seed) {
  const uint8_t *data = (const uint8_t *)key;
  const int nblocks = len / 4;

  uint32_t h1 = seed;
  uint32_t c1 = 0xcc9e2d51;
  uint32_t c2 = 0x1b873593;

  const uint32_t *blocks = (const uint32_t *)(data + nblocks * 4);
  for (int i = -nblocks; i; i++) {
    uint32_t k1 = getblock(blocks, i);
    bmix(&h1, &k1, &c1, &c2);
  }

  const uint8_t *tail = (const uint8_t *)(data + nblocks * 4);

  uint32_t k1 = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
  switch (len & 3) {
  case 3:
    k1 ^= tail[2] << 16;
  case 2:
    k1 ^= tail[1] << 8;
  case 1:
    k1 ^= tail[0];
    k1 *= c1;
    k1 = rotl32(k1, 16);
    k1 *= c2;
    h1 ^= k1;
  };
#pragma GCC diagnostic pop

  h1 ^= len;
  h1 = fmix(h1);
  return h1;
}

void __hm_sanity_check(hashmap_t *hm) {
  assert(hm != NULL);
  assert(hm->eq != NULL);
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

  unsigned size = hashentry_vector_size(hm->entries);
  uint32_t initial =
      __hm_murmur3((const void *)&key, sizeof(key), __HM_SEED1) % size;
  uint32_t delta =
      __hm_murmur3((const void *)&key, sizeof(key), __HM_SEED2) % size;
  if (delta == 0)
    delta = 1;

  for (unsigned x = 0;; x++) {
    unsigned hash = (initial + x * delta) % size;
    hash_entry_t current = hashentry_vector_at(hm->entries, hash);

    if (current.state == DELETED ||
        (current.state == USED && !hm->eq(current.key, key)))
      continue;

    if (current.state == EMPTY)
      // Im Moment geben wir einfach einen OOB Index zurück, könnte
      // wahrscheinlich besser gemacht werden. Weil das hier aber interner
      // Library Code ist, ist das nicht so schlimm.
      return size;

    return hash;
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

  unsigned size = hashentry_vector_size(hm->entries);
  uint32_t initial =
      __hm_murmur3((const void *)&key, sizeof(key), __HM_SEED1) % size;
  uint32_t delta =
      __hm_murmur3((const void *)&key, sizeof(key), __HM_SEED2) % size;
  if (delta == 0)
    delta = 1;

  for (unsigned x = 0;; x++) {
    unsigned hash = (initial + x * delta) % size;

    hash_entry_t current = hashentry_vector_at(hm->entries, hash);
    if (current.state == EMPTY || current.state == DELETED)
      return hash;
  }
}

hashmap_t *hashmap_create(eq_t eq) {
  hashmap_t *hm = malloc(sizeof(hashmap_t));
  assert(hm != NULL);

  hm->entries = hashentry_vector_create();
  hm->used_count = 0;
  hm->eq = eq;

  __hm_sanity_check(hm);
  __hm_rehash(hm);

  return hm;
}

unsigned hashmap_size(hashmap_t *hm) {
  __hm_sanity_check(hm);
  return hm->used_count;
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

  // NOTE: Es ist sehr wichtig, nach jedem insert zu prüfen, ob die HashMap
  // vergrößert werden muss. Ansonsten kann es passieren, dass die HashMap voll
  // wird und wir beim nächsten Read Lookup in einer Endlosschleife landen, weil
  // die Abbruchkondition (leerer Slot) nicht mehr erfüllt werden kann.
  if (hm->used_count * 100 >=
      hashentry_vector_size(hm->entries) * LOAD_FACTOR_IN_PERCENT)
    __hm_rehash(hm);
}

void hashmap_remove(hashmap_t *hm, int key) {
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

int_vector_t *hashmap_keys(hashmap_t *hm) {
  __hm_sanity_check(hm);

  int_vector_t *keys = int_vector_create_with_capacity(hashmap_size(hm));

  for (unsigned i = 0; i < hashentry_vector_size(hm->entries); i++) {
    hash_entry_t entry = hashentry_vector_at(hm->entries, i);
    if (entry.state != USED) {
      continue;
    }

    int_vector_append(keys, entry.key);
  }

  return keys;
}

int hashmap_get(hashmap_t *hm, int key) {
  __hm_sanity_check(hm);

  unsigned index = __hm_lookup_for_reading(hm, key);
  assert(index < hashentry_vector_size(hm->entries));

  hash_entry_t entry = hashentry_vector_at(hm->entries, index);
  return entry.value;
}

void hashmap_clear(hashmap_t *hm) {
  __hm_sanity_check(hm);

  hash_entry_t initial = {.state = EMPTY};
  for (unsigned i = 0; i < hashentry_vector_size(hm->entries); i++) {
    hashentry_vector_insert_at(hm->entries, i, initial);
  }

  hm->used_count = 0;
}

void hashmap_destroy(hashmap_t *hm) {
  __hm_sanity_check(hm);
  hashentry_vector_destroy(hm->entries);
  free(hm);
}
