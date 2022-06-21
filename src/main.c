#include "lib/time_util.h"
#include "src/transport.h"
#include "src/virtual_transport.h"
#include "boilerplate.h"
#include "interface.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

bool shutdown_flag = false;
void kill_handler(int signo) {
  (void)signo;
  shutdown_flag = true;
}

int main(int argc, char *argv[]) {
  // TODO interface initialize
  msg_priority_queue_t* msg_queue = msg_priority_queue_create();
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
  uint8_t send_buffer[255];

  int msg_number = 0;
  while (!shutdown_flag) {
    unsigned length = sizeof(receive_buffer);
    if (listen(receive_buffer, &length, 100)) {
      printf("%s\n", receive_buffer);
    }

    msg_number++;
    snprintf((char *)send_buffer, 255,
             "Hello bröther this is node ID_TBD with msg number %d",
             msg_number);
    send_packet(send_buffer, strlen((char *)send_buffer));
  }

  // TODO merge beaglebone communication with virtual
    /*send_ready(msg_queue);*/

   /*TODO shutdown stuff*/
  /*pthread_kill(&user_input_thread, 0);*/
  /*spi_shutdown();*/
  return 0;
}

