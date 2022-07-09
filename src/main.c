#include "lib/time_util.h"
#include "src/interface/boilerplate.h"
#include "src/interface/interface.h"
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
  // TODO interface initialize
  msg_priority_queue_t *msg_queue = msg_priority_queue_create();
  assert(msg_queue != NULL);
  pthread_t user_input_thread;
  int user_input_thread_id =
      pthread_create(&user_input_thread, NULL, collect_user_input, msg_queue);

  if (!transport_initialize()) {
    fprintf(stderr, "Couldn't initialize transport.\n");
    exit(EXIT_FAILURE);
  }

  struct sigaction sa;
  sa.sa_handler = kill_handler;
  sigaction(SIGINT, &sa, NULL);
  uint8_t receive_buffer[255];
  uint16_t freqs[2] = {850, 920};
  unsigned freq_idx = 0;

  message_t msg;

  while (!shutdown_flag) {
    if (transport_receive_message(&msg)) {
      printf("Received message!\n");
      transport_change_frequency(freqs[++freq_idx % 2]);
    }
  }

  /*TODO shutdown stuff*/
  /*pthread_kill(&user_input_thread, 0);*/
  /*spi_shutdown();*/
  return 0;
}
