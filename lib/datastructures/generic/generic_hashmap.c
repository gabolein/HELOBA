#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>

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