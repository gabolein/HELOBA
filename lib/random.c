#include "lib/random.h"
#include <assert.h>
#include <stdlib.h>
#include <time.h>

// TODO: Wenn wir noch genug Zeit haben, sollten wir einen Test schreiben, der
// prüft ob die Distribution wirklich uniform ist.
size_t random_number_between(size_t min, size_t max) {
  assert(min <= max);
  size_t interval = max - min;
  size_t cap = interval * (RAND_MAX / interval);
  size_t result = 0;

  srandom(time(NULL));

  do {
    result = random();
  } while (result > cap);

  return (result % interval) + min;
}