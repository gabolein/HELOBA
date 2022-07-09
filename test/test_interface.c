#include "src/interface/boilerplate.h"
#include "src/interface/interface.h"
#include <criterion/criterion.h>
#include <pty.h>
#include <sys/socket.h>

Test(interface, input_normal) {
  msg_priority_queue_t *msg_queue = msg_priority_queue_create();
  int ptyfd;
  int pid = forkpty(&ptyfd, 0, 0, 0);
  if (!pid) {
    collect_user_input(msg_queue);
  }

  char string[] = "Hello World";
  send(ptyfd, string, strlen(string) + 1, 0);
  msg *first_msg = msg_priority_queue_peek(msg_queue);

  cr_assert(first_msg == NULL);
  cr_assert(strcmp((char *)first_msg->data, string) == 0);
  cr_assert(first_msg->len == strlen(string) + 1);
  /*puts("test 1 done");*/
}

Test(interface, scanf_long) {
  msg_priority_queue_t *msg_queue = msg_priority_queue_create();
  int ptyfd;
  int pid = forkpty(&ptyfd, 0, 0, 0);
  if (!pid) {
    collect_user_input(msg_queue);
  }

  char string[] =
      "b00000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000bs";
  send(ptyfd, string, strlen(string) + 1, 0);
  msg *first_msg = msg_priority_queue_peek(msg_queue);

  cr_assert(first_msg == NULL);
  cr_assert(first_msg->len == 255);
}
