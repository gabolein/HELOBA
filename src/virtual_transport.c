#define LOG_LEVEL DEBUG_LEVEL
#define LOG_LABEL "Virtual Transport"

#include "src/virtual_transport.h"
#include "lib/datastructures/generic/generic_hashmap.h"
#include "lib/logger.h"
#include "lib/time_util.h"
#include "src/protocol/message.h"
#include "src/protocol/routing.h"
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

unsigned short get_frequency_multicast_port(uint16_t frequency) {
  assert(frequency_is_valid(frequency));
  return htons(1024 + frequency);
}

struct ip_mreq mreq = {.imr_interface = {.s_addr = INADDR_ANY},
                       .imr_multiaddr = {0}};

struct sockaddr_in comm_addr;

bool virtual_change_frequency(uint16_t frequency) {
  assert(frequency_is_valid(frequency));

  if (mreq.imr_multiaddr.s_addr != 0) {
    if (setsockopt(virt_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq,
                   sizeof(mreq)) < 0) {
      perror("setsockopt");
      return false;
    }

    close(virt_fd);
  }

  if ((virt_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    dbgln("Couldn't create virtual socket!");
    return false;
  }

  if (setsockopt(virt_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &(int){1},
                 sizeof(int)) < 0) {
    dbgln("Couldn't enable multicast on loopback interface!");
    close(virt_fd);
    return false;
  }

  if (setsockopt(virt_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
    dbgln("Couldn't set SO_REUSEADDR for socket.");
    close(virt_fd);
    return false;
  }

  struct sockaddr_in sockaddr;
  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  sockaddr.sin_port = get_frequency_multicast_port(frequency);
  if (bind(virt_fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
    perror("Couldn't bind socket");
    close(virt_fd);
    return false;
  }

  // init comm_addr
  memset(&comm_addr, 0, sizeof(comm_addr));
  comm_addr.sin_family = AF_INET;
  comm_addr.sin_port = get_frequency_multicast_port(frequency);
  struct in_addr addr = get_frequency_multicast_addr(frequency);
  comm_addr.sin_addr = addr;
  mreq.imr_multiaddr = addr;

  if (setsockopt(virt_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) <
      0) {
    perror("setsockopt");
    return false;
  }

  dbgln("Changed frequency to %u.", frequency);
  return true;
}

bool virtual_send_packet(uint8_t *buffer, unsigned length) {
  if (sendto(virt_fd, buffer, length, 0, (struct sockaddr *)&comm_addr,
             sizeof(comm_addr)) < 0) {
    dbgln("Couldn't send %u bytes to fd=%i: %s", length, virt_fd,
          strerror(errno));
    return false;
  }

  dbgln("Sent packet.");
  return true;
}

bool virtual_receive_packet(uint8_t *buffer, unsigned *length) {
  struct pollfd fds;
  fds.fd = virt_fd;
  fds.events = POLLIN;

  // NOTE: Timeout sollte wahrscheinlich kleiner sein
  switch (poll(&fds, 1, 0)) {
  case -1:
    dbgln("Couldn't poll fd=%i: %s", virt_fd, strerror(errno));
    return false;
  case 0:
    /*dbgln("receive packet: timeout");*/
    return false;
  default: {
    ssize_t ret;
    if ((ret = recv(virt_fd, buffer, MAX_MSG_LEN, 0)) < 0) {
      dbgln("Couldn't receive message: %s", strerror(errno));
    }

    *length = ret;
    return true;
  }
  }
}

bool virtual_get_id(uint8_t *out) {
  assert(sizeof(pid_t) <= MAC_SIZE);
  pid_t pid = getpid();
  memcpy(out, &pid, sizeof(pid));
  memset(out + sizeof(pid), 0, MAC_SIZE - sizeof(pid_t));
  return true;
}

// NOTE: Es kann sein, dass wir unsere eigenen gesendeten Nachrichten empfangen,
// diese Funktion sollte also nur aufgerufen werden, bevor wir etwas gesendet
// haben, um mit der Radio Variante konsistente Ergebisse zu liefern
bool virtual_channel_active() {
  struct pollfd fds;
  fds.fd = virt_fd;
  fds.events = POLLIN;

  int ret;
  // NOTE: Timeout sollte wahrscheinlich kleiner sein
  if ((ret = poll(&fds, 1, 0)) < 0) {
    perror("poll");
    return false;
  }

  return ret != 0;
}
