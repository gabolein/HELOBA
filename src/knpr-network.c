#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <poll.h>


#define MAX_DATA_LEN 10
#define MAX_POLL_FDS MAX_CLIENT_COUNT + 2
#define CHECK_ERR(predicate, msg) if (predicate) { \
                                    perror("error: " msg); \
                                    close(multi_sock_fd); \
                                    return -1; }

int multi_sock_fd;
static struct ip_mreqn multi_group;
static struct sockaddr multi_sockaddr;



int create_multi_socket(const char *addr_str, uint16_t porth)
{
  int retval;
  multi_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
  CHECK_ERR(multi_sock_fd < 0, "create udp socket")

  // allow multiple processes to bind to the same address
  int sockopt_val = 1;
  retval = setsockopt(multi_sock_fd, SOL_SOCKET, SO_REUSEADDR,
                      &sockopt_val, sizeof(sockopt_val));
  CHECK_ERR(retval != 0, "sockopt SO_REUSEADDR") 

  // bind socket to INADDR_ANY and given port
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(porth);
  retval = bind(multi_sock_fd, (struct sockaddr *) &addr, sizeof(addr));
  CHECK_ERR(retval != 0, "bind")

  // allow loopback of multicast packets
  sockopt_val = 1;
  retval = setsockopt(multi_sock_fd, IPPROTO_IP, IP_MULTICAST_LOOP,
                      &sockopt_val, sizeof(sockopt_val));
  CHECK_ERR(retval != 0, "sockopt IP_MULTICAST_LOOP")

  // set ttl to 1 (the default value)
  sockopt_val = 1;
  retval = setsockopt(multi_sock_fd, IPPROTO_IP, IP_MULTICAST_TTL,
      &sockopt_val, sizeof(sockopt_val));
  CHECK_ERR(retval != 0, "sockopt IP_MULTICAST_TTL")


  //join the multicast group
  retval =
    inet_pton(AF_INET, addr_str, &multi_group.imr_multiaddr.s_addr);
  CHECK_ERR(retval != 1, "address conversion")
  multi_group.imr_address.s_addr = htonl(INADDR_ANY);
  multi_group.imr_ifindex = 0;
  retval = setsockopt(multi_sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                      &multi_group, sizeof(multi_group));
  CHECK_ERR(retval != 0, "sockopt IP_ADD_MEMBERSHIP")

  // initialize a struct inetaddr for use in later sendto's
  struct sockaddr_in *multi_sockaddr_ptr =
    (struct sockaddr_in *) &multi_sockaddr;
  memset(multi_sockaddr_ptr, 0, sizeof(struct sockaddr_in));
  multi_sockaddr_ptr->sin_family = AF_INET;
  inet_pton(AF_INET, addr_str, &multi_sockaddr_ptr->sin_addr.s_addr);
  multi_sockaddr_ptr->sin_port = htons(porth);

  return 0;
}

int receive_multicast_msg(char *buf, unsigned buf_size)
{
  struct sockaddr_in rx_addr;
  memset(&rx_addr, 0, sizeof(rx_addr));
  unsigned addr_len = sizeof(rx_addr);

  long count = recvfrom(multi_sock_fd, buf, buf_size, 0,
      (struct sockaddr *) &rx_addr, &addr_len);

  printf("Message received from address %s and port %hu\n",
         inet_ntoa(rx_addr.sin_addr), ntohs(rx_addr.sin_port));

  if(count > MAX_DATA_LEN)
  {
    fprintf(stderr,
        "error: too long Message with length %ld received\n", count);
    count = -1;
  }
  return (int) count;
}

int close_multi_socket()
{
  int retval = setsockopt(multi_sock_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                          &multi_group, sizeof(multi_group));
  if (retval != 0)
  {
    perror("Error setting sockopt IP_DROP_MEMBERSHIP");
    retval = -1;
  }
  else retval = 0;

  close(multi_sock_fd);
  return retval;
}

int main()
{
  create_multi_socket("239.0.0.1", 1230);
  char buff[MAX_DATA_LEN];
  while(sleep(1) == 0)
  {
    int cnt = receive_multicast_msg(buff, MAX_DATA_LEN);
    if (cnt > 0)
      printf("%s\n", buff);
  }
  printf("closing\n");
    
  close_multi_socket();
  return 0;
}
