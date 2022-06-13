#include "src/virtual_transport.h"
#include "lib/datastructures/generic/generic_hashmap.h"
#include "src/state.h"
#include <arpa/inet.h>
#include <assert.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

bool eq(uint32_t a, uint32_t b) { return a == b; }

unsigned h1(uint32_t key) {
  key = ((key >> 16) ^ key) * 0x45d9f3b;
  key = ((key >> 16) ^ key) * 0x45d9f3b;
  key = (key >> 16) ^ key;
  return key;
}

unsigned h2(uint32_t key) {
  key += ~(key << 15);
  key ^= (key >> 10);
  key += (key << 3);
  key ^= (key >> 6);
  key += ~(key << 11);
  key ^= (key >> 16);
  return key;
}

MAKE_SPECIFIC_HASHMAP_HEADER(uint16_t, int, freqsocket)
MAKE_SPECIFIC_HASHMAP_SOURCE(uint16_t, int, freqsocket, h1, h2, eq)

static uint8_t recv_buffer[UINT8_MAX];
static freqsocket_hashmap_t *global_socket_map;
static struct sockaddr_ll global_broadcast_addr;

// NOTE: ich habe keine Ahnung ob das alle unterstützten Frequenzen sind
bool frequency_is_valid(uint16_t frequency) {
  return frequency >= 800 && frequency <= 950;
}

bool virtid_is_valid(uint8_t virtid) { return virtid < 10; }

bool virtual_change_frequency(uint16_t frequency, uint8_t virtid) {
  assert(frequency_is_valid(frequency));
  assert(virtid_is_valid(virtid));

  int dummy = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  struct ifreq req;
  memset(&req, 0, sizeof(req));

  snprintf(req.ifr_name, IFNAMSIZ, "eth%u.%u", frequency, virtid);
  if (ioctl(dummy, SIOCGIFFLAGS, &req) < 0) {
    fprintf(stderr, "Please run setup_broadcast_domains.sh to setup all "
                    "ethernet interfaces needed for radio emulation!\n");
    close(dummy);
    return false;
  }

  if ((req.ifr_flags & IFF_UP) == 0) {
    printf(
        "Please call virtual_interfaces_initialize() before this function!\n");
    close(dummy);
    return false;
  }

  close(dummy);
  state.frequency = frequency;
  return true;
}

bool virtual_interfaces_initialize(uint8_t virtid) {
  uint16_t available_frequencies[] = {840, 860, 880, 900, 920};

  if (!(global_socket_map = freqsocket_hashmap_create())) {
    return false;
  }

  struct ifreq ifr;
  for (size_t i = 0; i < 5; i++) {
    printf("Creating socket connecting to interface eth%u.%u\n",
           available_frequencies[i], virtid);
    int node_socket;
    if ((node_socket = socket(AF_PACKET, SOCK_DGRAM, htons(0x88b5))) == -1) {
      perror("socket");
      return false;
    }

    char if_name[IFNAMSIZ];
    snprintf(if_name, IFNAMSIZ, "eth%u.%u", available_frequencies[i], virtid);
    strcpy(ifr.ifr_name, if_name);

    if (ioctl(node_socket, SIOCGIFINDEX, &ifr) == -1) {
      perror("ioctl");
      return false;
    }

    freqsocket_hashmap_insert(global_socket_map, available_frequencies[i],
                              node_socket);
  }

  global_broadcast_addr.sll_family = AF_PACKET;
  global_broadcast_addr.sll_ifindex = ifr.ifr_ifindex;
  global_broadcast_addr.sll_halen = ETHER_ADDR_LEN;
  global_broadcast_addr.sll_protocol = htons(0x88b5);
  /*11111111111... eth broadcast*/
  uint8_t dest[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  memcpy(global_broadcast_addr.sll_addr, dest, ETHER_ADDR_LEN);

  return true;
}

bool virtual_send_packet(uint8_t *buffer, unsigned length) {
  int node_socket = freqsocket_hashmap_get(global_socket_map, state.frequency);

  if (sendto(node_socket, buffer, length, 0,
             (struct sockaddr *)&global_broadcast_addr,
             sizeof(global_broadcast_addr)) < 0) {
    perror("sendto error");
    return false;
  }

  return true;
}

bool virtual_receive_packet(uint8_t *buffer, unsigned *length) {
  int node_socket = freqsocket_hashmap_get(global_socket_map, state.frequency);
  struct pollfd fd;
  fd.fd = node_socket;
  fd.events = POLLIN;

  // NOTE: Timeout sollte wahrscheinlich kleiner sein
  switch (poll(&fd, 1, 100)) {
  case -1:
    perror("poll");
    return false;
  case 0:
    return false;
  default:
    recv(node_socket, buffer, *length, 0);
    *length = recv_buffer[0];

    return true;
  }
}