#include "src/virtual_transport.h"
#include "lib/time_util.h"
#include "lib/datastructures/generic/generic_hashmap.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <assert.h>
#include <errno.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define IPADDR_MAX_LEN 16
#define MULTICAST_FMT "224.0.%u.%u"
static char multicast_addr[IPADDR_MAX_LEN] = {0};
int virt_fd;

bool frequency_is_valid(uint16_t frequency) {
  return frequency >= 800 && frequency <= 950;
}

struct in_addr get_frequency_multicast_addr(uint16_t frequency) {
  assert(frequency_is_valid(frequency));

  struct in_addr out;
  snprintf(multicast_addr, IPADDR_MAX_LEN, MULTICAST_FMT,
           (frequency & 0xFF00) >> 8, (frequency & 0x00FF) >> 0);
  inet_aton(multicast_addr, &out);
  return out;
}

struct ip_mreq mreq = {.imr_interface = {.s_addr = INADDR_ANY},
                       .imr_multiaddr = {0}};

struct sockaddr_in comm_addr;

bool virtual_change_frequency(uint16_t frequency) {
  assert(frequency_is_valid(frequency));

  if (mreq.imr_multiaddr.s_addr != 0)
    setsockopt(virt_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));

  struct in_addr addr = get_frequency_multicast_addr(frequency);
  comm_addr.sin_addr = addr;
  mreq.imr_multiaddr = addr;

  if (setsockopt(virt_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) <
      0) {
    fprintf(stderr, "Couldn't join multicast group for frequency %u",
            frequency);
    // NOTE: muss hier direkt die socket geschlossen werden
    close(virt_fd);
    return false;
  }

  return true;
}

bool virtual_transport_initialize() {
  if ((virt_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    fprintf(stderr, "Couldn't create virtual socket!\n");
    return false;
  }

  if (setsockopt(virt_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &(int){1},
                 sizeof(int)) < 0) {
    fprintf(stderr, "Couldn't enable multicast on loopback interface!\n");
    close(virt_fd);
    return false;
  }

  if (setsockopt(virt_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
    fprintf(stderr, "Couldn't set SO_REUSEADDR for socket.\n");
    close(virt_fd);
    return false;
  }

  struct sockaddr_in sockaddr;
  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  sockaddr.sin_port = htons(1337);

  if (bind(virt_fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
    perror("Couldn't bind socket");
    close(virt_fd);
    return false;
  }
  
  // init comm_addr
  memset(&comm_addr, 0, sizeof(comm_addr));
  comm_addr.sin_family = AF_INET;
  comm_addr.sin_port = htons(1337);

  // per Default starten alle auf Frequenz 850
  virtual_change_frequency(850);
  return true;
}

bool virtual_send_packet(uint8_t *buffer, unsigned length) {
  if (sendto(virt_fd, buffer, length, 0, (struct sockaddr*)&comm_addr, sizeof(comm_addr)) < 0) {
    fprintf(stderr, "Couldn't send %u bytes to fd=%i:\n", length, virt_fd);
    fprintf(stderr, "%s\n", strerror(errno));
    return false;
  }

  return true;
}

bool virtual_listen(uint8_t *buffer, unsigned *length, unsigned listen_ms) {
  struct timespec start_time;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
  while(!hit_timeout(listen_ms, &start_time)){
    if(virtual_receive_packet(buffer, length)){
      return true;
    }
  }
  return false;
}

bool virtual_receive_packet(uint8_t *buffer, unsigned *length) {
  struct pollfd fds;
  fds.fd = virt_fd;
  fds.events = POLLIN;

  // NOTE: Timeout sollte wahrscheinlich kleiner sein
  switch (poll(&fds, 1, 100)) {
  case -1:
    fprintf(stderr, "Couldn't poll fd=%i:\n", virt_fd);
    fprintf(stderr, "%s\n", strerror(errno));
    return false;
  case 0:
    return false;
  default:
    // TODO import definition of packet length from somewhere else, remove magic number
    if (recv(virt_fd, buffer, 255, 0) < 0) {
      fprintf(stderr, "Couldn't receive message:\n");
      fprintf(stderr, "%s\n", strerror(errno));
    }
    *length = buffer[0];
    return true;
  }
}

bool virtual_get_id(uint8_t *out) {
  pid_t pid = getpid();
  memcpy(out, &pid, sizeof(pid));
  return true;
}
