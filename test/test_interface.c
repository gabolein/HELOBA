#include <criterion/criterion.h>
#include <pty.h>
#include <sys/socket.h>
#include "boilerplate.h"
#include "interface.h"

Test(interface, scanf_normal){
  queue* msg_queue = create_queue();
  int ptyfd;
  int pid = forkpty(&ptyfd, 0, 0, 0);
  if(!pid) {
    collect_user_input(msg_queue);
  } 
  
  char string[] = "Hello World";
  send(ptyfd, string, strlen(string)+1, 0);
  queue_elem* elem = peek_queue(msg_queue);

  cr_assert(elem->next == NULL);
  msg* message = elem->val;
  cr_assert(strcmp((char*)message->data, string) == 0);
  cr_assert(message->len == strlen(string)+1);
  puts("test 1 done");
}

Test(interface, scanf_long){
  queue* msg_queue = create_queue();
  int ptyfd;
  int pid = forkpty(&ptyfd, 0, 0, 0);
  if(!pid) {
    collect_user_input(msg_queue);
  } 
  
  char string[] = "b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000bs";
  send(ptyfd, string, strlen(string)+1, 0);
  queue_elem* elem = peek_queue(msg_queue);

  cr_assert(elem->next == NULL);
  msg* message = elem->val;
  cr_assert(message->len == 255);

}

Test(queue, easy){
  queue* msg_queue = create_queue();
  cr_assert(msg_queue != NULL);

  int value = 42;
  queue_elem elem = {&value, (void*)&value};
  enqueue(msg_queue, &elem);
  cr_assert(msg_queue->first == &elem);
  cr_assert(msg_queue->last == &elem);
  cr_assert(elem.next == NULL);

  queue_elem elem2 = {&value, (void*)&value};
  enqueue(msg_queue, &elem2);
  cr_assert(msg_queue->first == &elem);
  cr_assert(msg_queue->last == &elem2);
  cr_assert(elem.next == &elem2);
  cr_assert(elem2.next == NULL);

  queue_elem* return_elem = dequeue(msg_queue);
  cr_assert(return_elem == &elem);
  cr_assert(msg_queue->first == &elem2);
  cr_assert(msg_queue->last == &elem2);

  return_elem = dequeue(msg_queue);
  cr_assert(return_elem == &elem2);
  cr_assert(msg_queue->first == NULL);
  cr_assert(msg_queue->last == NULL);
}
