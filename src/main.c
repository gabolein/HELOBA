#define LOG_LEVEL DEBUG_LEVEL
#define LOG_LABEL "Main"

#include "lib/logger.h"
#include "lib/time_util.h"
#include "src/event_loop.h"
#include "src/interface/interface.h"
#include "src/state.h"
#include "src/transport.h"
#include "src/virtual_transport.h"
#include "src/protocol/cache.h"
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool shutdown_flag = false;
void kill_handler(int signo) {
  (void)signo;
  shutdown_flag = true;
}

int main() {
  srandom(time(NULL));

  initialize_global_state();
  cache_initialize();

  if (!transport_initialize()) {
    dbgln("Couldn't initialize transport.");
    exit(EXIT_FAILURE);
  }

  interface_initialize();

  struct sigaction sa;
  sa.sa_handler = kill_handler;
  sigaction(SIGINT, &sa, NULL);

  event_loop_initialize();
  while (!shutdown_flag) {
    event_loop_run();
  }

  /*TODO shutdown stuff*/
  /*pthread_kill(&user_input_thread, 0);*/
  /*spi_shutdown();*/
  return 0;
}
