#define _POSIX_C_SOURCE 199309L
#include <criterion/criterion.h>
#include <time.h>
#include <unistd.h>
#include "backoff.h"
#include "boilerplate.h"

Test(backoff, no_msg){
  queue* q = create_queue();
  cr_assert(!send_ready(q));
}

Test(backoff, new_backoff_simple){
  backoff_struct backoff = {0, 0, 0};
  for(int i = 0; i < 100; i++){
    set_new_backoff(&backoff);
    cr_assert(backoff.backoff_ms == 0 || backoff.backoff_ms == 1);
    backoff.attempts = 0;
  }
}

Test(backoff, timeout){
  backoff_struct backoff = {0, 100, 0};
  clock_gettime(CLOCK_MONOTONIC_RAW, &backoff_struct.start_backoff);
  cr_assert(!check_backoff_timeout(&backoff));
  sleep(1);
  cr_assert(check_backoff_timeot(&backoff));
}
  

