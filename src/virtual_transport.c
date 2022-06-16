#include "src/virtual_transport.h"
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

bool virtual_change_frequency(uint16_t frequency) {
  assert(frequency_is_valid(frequency));

  if (mreq.imr_multiaddr.s_addr != 0)
    setsockopt(virt_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));

  mreq.imr_multiaddr = get_frequency_multicast_addr(frequency);

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
  if ((virt_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    fprintf(stderr, "Couldn't create virtual socket!\n");
    return false;
  }

  if (setsockopt(virt_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &(int){0},
                 sizeof(int)) < 0) {
    fprintf(stderr, "Couldn't enable multicast on loopback interface!\n");
    close(virt_fd);
    return false;
  }

  if (setsockopt(virt_fd, IPPROTO_IP, SO_REUSEADDR, &(int){1}, sizeof(int))) {
    fprintf(stderr, "Couldn't set SO_REUSEADDR for socket.\n");
    close(virt_fd);
    return false;
  }

  // per Default starten alle auf Frequenz 850
  virtual_change_frequency(850);
  return true;
}

bool virtual_send_packet(uint8_t *buffer, unsigned length) {
  if (send(virt_fd, buffer, length, 0) < 0) {
    fprintf(stderr, "Couldn't send %u bytes to fd=%i:\n", length, virt_fd);
    fprintf(stderr, "%s\n", strerror(errno));
    return false;
  }

  return true;
}

bool virtual_receive_packet(uint8_t *buffer, unsigned *length) {
  struct pollfd fds;
  fds.fd = virt_fd;
  fds.events = POLLIN;

  // NOTE: Timeout sollte wahrscheinlich kleiner sein
  switch (poll(&fds, 1, 100)) {
  case -1:
    fprintf(stderr, "Couldn't poll fd=%i:", virt_fd);
    fprintf(stderr, "%s\n", strerror(errno));
    return false;
  case 0:
    return false;
  default:
    if (recv(virt_fd, buffer, *length, 0) < 0) {
      fprintf(stderr, "Couldn't receive message:\n");
      fprintf(stderr, "%s\n", strerror(errno));
    }
    *length = buffer[0];
    return true;
  }
}