#include "lib/time_util.h"
#include "src/transport.h"
#include "src/virtual_transport.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool shutdown_flag = false;
void kill_handler(int signo) {
  (void)signo;
  shutdown_flag = true;
}

int main(int argc, char *argv[]) {
  struct sigaction sa;
  sa.sa_handler = kill_handler;
  sigaction(SIGINT, &sa, NULL);

#if defined(VIRTUAL)
  if (!virtual_transport_initialize()) {
    fprintf(stderr, "Couldn't initialize the virtual transport.\n");
    exit(EXIT_FAILURE);
  }
#endif

  uint8_t receive_buffer[255];
  uint8_t send_buffer[255];

  int msg_number = 0;
  while (!shutdown_flag) {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    unsigned length = sizeof(receive_buffer);
    if (receive_packet(receive_buffer, &length)) {
      printf("%s\n", receive_buffer);
    }

    msg_number++;
    snprintf((char *)send_buffer, 255,
             "Hello bröther this is node ID_TBD with msg number %d",
             msg_number);
    send_packet(send_buffer, strlen((char *)send_buffer));
  }
}
