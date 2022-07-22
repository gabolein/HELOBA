#define LOG_LEVEL WARNING_LEVEL
#define LOG_LABEL "Virtual Transport"

#include "src/virtual_transport.h"
#include "lib/datastructures/generic/generic_hashmap.h"
#include "lib/logger.h"
#include "lib/time_util.h"
#include "src/config.h"
#include "src/protocol/message.h"
#include "src/protocol/message_parser.h"
#include "src/state.h"
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
  return frequency >= FREQUENCY_BASE && frequency <= FREQUENCY_CEILING;
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
  memset(out, 0, MAC_SIZE);
  memcpy(out, &pid, sizeof(pid));
  return true;
}

static uint8_t active_buf[MAX_MSG_LEN];

// NOTE: Wir müssen hier Nachrichten parsen, weil wir mit UDP Multicast unsere
// eigenen Nachrichten empfangen. Diese Nachricht müssen wir rausfiltern, wenn
// wir wirklich prüfen wollen, ob der Channel aktiv ist, d.h. jemand anderes als
// wir sendet.
bool virtual_channel_active(unsigned timeout_ms) {
  struct pollfd fds;
  fds.fd = virt_fd;
  fds.events = POLLIN;

  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
  while (!hit_timeout(timeout_ms, &start)) {
    int ret = poll(&fds, 1, 0);
    if (ret <= 0) {
      if (ret < 0) {
        perror("poll");
      }

      continue;
    }

    ret = recv(virt_fd, active_buf, MAX_MSG_LEN, MSG_PEEK);
    if (ret < 0) {
      dbgln("Couldn't receive message: %s", strerror(errno));
      return true;
    }

    message_t msg = unpack_message(active_buf, ret);
    if (!message_is_valid(&msg)) {
      warnln("Received invalid message, still assuming channel is active.");
      return true;
    }

    if (!routing_id_equal(msg.header.sender_id, gs.id)) {
      return true;
    } else {
      recv(virt_fd, active_buf, MAX_MSG_LEN, 0);
    }
  }

  return false;
}
