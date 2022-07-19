#include "lib/random.h"
#include <criterion/criterion.h>
#include <math.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void setup(void) { srandom(time(NULL)); }

Test(random, number_between_interval, .init = setup) {
  for (unsigned i = 0; i < 100000; i++) {
    size_t min = random_number_between(0, RAND_MAX);
    size_t max = random_number_between(0, RAND_MAX);

    if (min >= max || (max - min) > RAND_MAX) {
      continue;
    }

    size_t random = random_number_between(min, max);
    cr_assert(min <= random);
    cr_assert(random < max);
  }
}

Test(random, number_between_bounds, .init = setup, .signal = SIGABRT) {
  random_number_between(0, SIZE_MAX);
}

static unsigned slot_count = 100;
static unsigned nums_per_slot = 10000;

Test(random, number_between_distribution, .init = setup) {
  size_t slots[slot_count];
  memset(slots, 0, sizeof(slots));

  for (unsigned i = 0; i < slot_count * nums_per_slot; i++) {
    slots[random_number_between(0, slot_count)]++;
  }

  for (unsigned i = 0; i < slot_count; i++) {
    cr_assert(fabs(1.0 - (float)slots[i] / (float)nums_per_slot) <= 0.03);
  }
}