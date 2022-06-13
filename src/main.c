#define _POSIX_C_SOURCE 199309L
#include "src/frequency.h"
#include "src/time_util.h"
#include <signal.h>
#include "packet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool shutdown_flag = false;
void kill_handler(int signo) {
  (void)signo;
  shutdown_flag = true;
}

int main(int argc, char* argv[]){
  if (argc != 2){
    puts("Please provide node ID as parameter.");
    return 0;
  }

  uint32_t node_id = atoi(argv[1]);
  printf("Starting node with ID %u\n", node_id);

  struct sigaction sa;
  sa.sa_handler = kill_handler;
  sigaction(SIGINT, &sa, NULL);

  init_virtual_interfaces(node_id);
  uint8_t receive_buffer[255];
  uint8_t send_buffer[255];
  

  int msg_number = 0;
  while(!shutdown_flag){
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    while(!hit_timeout(100, &start)){
      if(receive_packet_virtual(receive_buffer, global_frequency)){
        printf("%s\n", receive_buffer);
      }
    }

    msg_number++;
    snprintf((char*)send_buffer, 255, "Hello bröther this is node %u with msg number %d", node_id, msg_number);
    send_packet_virtual(send_buffer, global_frequency);
  }
}
