#include "lib/time_util.h"
#include "src/interface/interface.h"
#include "src/transport.h"
#include "src/virtual_transport.h"
#include "src/state.h"
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
  interface_initialize();

  if (!transport_initialize()) {
    fprintf(stderr, "Couldn't initialize transport.\n");
    exit(EXIT_FAILURE);
  }

  initialize_global_state();

  struct sigaction sa;
  sa.sa_handler = kill_handler;
  sigaction(SIGINT, &sa, NULL);
  uint8_t receive_buffer[255];

  while (!shutdown_flag) {
    sleep_ms(1000);
    interface_do_action();
  }

  /*TODO shutdown stuff*/
  /*pthread_kill(&user_input_thread, 0);*/
  /*spi_shutdown();*/
  return 0;
}
