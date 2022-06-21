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

// TODO integrate into receive_packet
/*void listen(){*/
  /*uint8_t packet[MAX_PACKET_LENGTH] = {0};*/
  /*if(detect_RSSI()){*/

    /*enable_preamble_detection();*/

    /*uint8_t recv_bytes;*/
    /*if((recv_bytes = receive_packet(packet)) == 0) {*/
      /*cc1200_cmd(SIDLE);*/
      /*cc1200_cmd(SFRX);*/
    /*}*/

    /*disable_preamble_detection();*/
  /*}*/
/*}*/

int main(int argc, char *argv[]) {
  // wohin? TODO init beaglebone
  /*if (spi_init() != 0) {*/
    /*printf("ERROR: Initialization failed\n");*/
    /*return -1;*/
  /*}*/
  /*cc1200_cmd(SRES);*/
  /*write_default_register_configuration();*/
  /*cc1200_cmd(SNOP);*/
  /*printf("CC1200 Status: %s\n", get_status_cc1200_str());*/


  // TODO interface initialize
  msg_priority_queue_t* msg_queue = msg_priority_queue_create();
  assert(msg_queue != NULL);
  pthread_t user_input_thread;
  int user_input_thread_id = 
    pthread_create(&user_input_thread, NULL, collect_user_input, msg_queue);
  
#if defined(VIRTUAL)
  if (!virtual_transport_initialize()) {
    fprintf(stderr, "Couldn't initialize the virtual transport.\n");
    exit(EXIT_FAILURE);
  }
#endif

  struct sigaction sa;
  sa.sa_handler = kill_handler;
  sigaction(SIGINT, &sa, NULL);
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

  // TODO merge beaglebone communication with virtual
  /*while(!shutdown_flag) {*/
    /*struct timespec start_time;*/
    /*clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);*/
    /*while(!hit_timeout(100, &start_time)){*/
      /*listen();*/
    /*}*/
    /*send_ready(msg_queue);*/
  /*}*/

   /*TODO shutdown stuff*/
  /*pthread_kill(&user_input_thread, 0);*/
  /*spi_shutdown();*/
  return 0;
}

