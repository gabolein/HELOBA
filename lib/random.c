#include "lib/random.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Returns a random number in the range [min, max[ with uniform distribution.
// Crashes if max >= min or if interval is larger than RAND_MAX.
size_t random_number_between(size_t min, size_t max) {
  assert(min < max);

  size_t result = 0;
  size_t interval = max - min;
  size_t cap = interval * (RAND_MAX / interval);
  assert(cap > 0);

  do {
    result = random();
  } while (result > cap);

  return (result % interval) + min;
}