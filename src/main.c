#include "lib/time_util.h"
#include "src/interface/interface.h"
#include "src/state.h"
#include "src/transport.h"
#include "src/virtual_transport.h"
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

  if (!transport_initialize()) {
    fprintf(stderr, "Couldn't initialize transport.\n");
    exit(EXIT_FAILURE);
  }

  interface_initialize();

  struct sigaction sa;
  sa.sa_handler = kill_handler;
  sigaction(SIGINT, &sa, NULL);

  while (!shutdown_flag) {
    sleep_ms(400);
    interface_do_action();
  }

  /*TODO shutdown stuff*/
  /*pthread_kill(&user_input_thread, 0);*/
  /*spi_shutdown();*/
  return 0;
}
