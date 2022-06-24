#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

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
uint32_t __ghm_murmur3(const void *key, int len, uint32_t seed) {
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

bool __ghm_is_prime(unsigned n) {
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

unsigned __ghm_next_prime(unsigned start) {
  for (unsigned c = start; c < UINT_MAX; c++) {
    if (__ghm_is_prime(c))
      return c;
  }

  assert(false);
}

#define LOAD_FACTOR_IN_PERCENT 75

bool __ghm_should_rehash(unsigned slots_used, unsigned current_size) {
  return slots_used * 100 >= current_size * LOAD_FACTOR_IN_PERCENT;
}