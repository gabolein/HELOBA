#include "src/frequency.h"
#include "src/state.h"
#include "src/time_util.h"
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
  if (argc != 2) {
    puts("Please provide node ID as parameter.");
    return 0;
  }

  state.virtid = atoi(argv[1]);
  printf("Starting node with ID %u\n", state.virtid);

  struct sigaction sa;
  sa.sa_handler = kill_handler;
  sigaction(SIGINT, &sa, NULL);

#if defined(VIRTUAL)
  // Es wäre besser, die Interfaces für die Frequenz automatisch in
  // virtual_change_frequency() zu erstellen, falls sie noch nicht existieren.
  // Dann bräuchten wir keine zusätzliche Setup-Funktion, die nur im VIRTUAL
  // Modus aufgerufen werden muss. Außerdem könnten wir dann beliebige
  // Frequenzen unterstützen.
  virtual_interfaces_initialize(node_id);
#endif

  uint8_t receive_buffer[255];
  uint8_t send_buffer[255];

  int msg_number = 0;
  while (!shutdown_flag) {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    while (!hit_timeout(100, &start)) {
      unsigned length = sizeof(receive_buffer);
      if (receive_packet(receive_buffer, &length)) {
        printf("%s\n", receive_buffer);
      }
    }

    msg_number++;
    snprintf((char *)send_buffer, 255,
             "Hello bröther this is node %u with msg number %d", state.virtid,
             msg_number);
    send_packet(send_buffer, strlen((char *)send_buffer));
  }
}
