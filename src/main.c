#define _POSIX_C_SOURCE 199309L
#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>
#include <SPIv1.h>
#include <signal.h>
#include <assert.h>
#include "registers.h"
#include "rssi.h"
#include "time_util.h"
#include "packet.h"
#include "boilerplate.h"
#include "interface.h"
#include "backoff.h"

bool shutdown_flag = false;

void kill_handler(int signo) {
  (void)signo;
  shutdown_flag = true;
}

void listen(){
  uint8_t packet[255] = {0};
  if(detect_RSSI()){

    enable_preamble_detection();

    uint8_t recv_bytes;
    if((recv_bytes = receive_packet(packet)) == 0) {
      cc1200_cmd(SIDLE);
      cc1200_cmd(SFRX);
    }

    disable_preamble_detection();
  }
}

int main() {
  if (spi_init() != 0) {
    printf("ERROR: Initialization failed\n");
    return -1;
  }

  struct sigaction sa;
  sa.sa_handler = kill_handler;
  sigaction(SIGINT, &sa, NULL);

  cc1200_cmd(SRES);
  write_default_register_configuration();
  cc1200_cmd(SNOP);
  printf("CC1200 Status: %s\n", get_status_cc1200_str());

  msg_priority_queue_t* msg_queue = msg_priority_queue_create();
  assert(msg_queue != NULL);
  pthread_t user_input_thread;
  pthread_create(&user_input_thread, NULL, collect_user_input, msg_queue);

  while(!shutdown_flag) {
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
    while(!hit_timeout(10, &start_time)){
      listen();
    }
    send_ready(msg_queue);
  }
}

