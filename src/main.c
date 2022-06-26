#include "lib/time_util.h"
#include "src/transport.h"
#include "src/virtual_transport.h"
#include "src/interface/boilerplate.h"
#include "src/interface/interface.h"
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

  while (!shutdown_flag) {
    unsigned length;
    if (listen(receive_buffer, &length, 100)) {
      printf("%s\n", receive_buffer);
    }
    
    if(msg_priority_queue_size(msg_queue)){
      msg* next_message = msg_priority_queue_peek(msg_queue);
      if(send_packet(next_message->data, next_message->len)){
        msg_priority_queue_pop(msg_queue);
      }
    }
  }

  /*TODO shutdown stuff*/
  /*pthread_kill(&user_input_thread, 0);*/
  /*spi_shutdown();*/
  return 0;
}

