#define _GNU_SOURCE
#include "src/interface/interface.h"
#include "src/protocol/message.h"
#include "src/state.h"
#include "src/transport.h"
#include "src/protocol/search.h"
#include "lib/datastructures/generic/generic_priority_queue.h"
#include "lib/time_util.h"
#include <poll.h>
#include <assert.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#define INTERFACE_COMMAND_COUNT 8
#define MAX_ARGC 3

typedef enum {
  SEARCHFOR,
  SEND,
  FREQ,
  LIST,
  GOTO,
  SPLIT,
  ID,
  UNKNOWN
} interface_commands;

typedef union {
  frequency_t freq;
  routing_id_t to_find;
} command_param_t;

typedef struct {
  bool set;
  interface_commands type;
  command_param_t command_param;
} interface_command_t;

interface_command_t command = {.set = false};
pthread_mutex_t interface_lock;

typedef bool (*interface_handler_f)(command_param_t param);

bool handle_freq(
    __attribute__ ((unused))command_param_t param){
  // FIXME where should frequency be set??
  printf("Current frequency: %u\n", gs.frequency);
  return true;
}

bool handle_list(
    __attribute__ ((unused))command_param_t param){
  // TODO get all nodes from hashmap and print them
  // need keys function for hashmap
  if (!(gs.flags & LEADER)) {
    printf("Node is not a leader and therefore has no list of nodes.\n");
    return false;
  }
  return true;
}

bool handle_id(
    __attribute__ ((unused))command_param_t param){
  // TODO does this work this way?
  printf("Node ID: %lx\n", (unsigned long) gs.id.optional_MAC);
  return true;
}

bool handle_split(
    __attribute__ ((unused))command_param_t param){
  // TODO call function that makes node split
  if (!(gs.flags & LEADER)) {
    printf("Node is not a leader and therefore cannot perform split.\n");
    return false;
  }
  return true;
}

bool handle_searchfor(command_param_t param){
  return search_for(param.to_find);
}

bool handle_goto(command_param_t param){
  // TODO unregister
  transport_change_frequency(param.freq);
  // TODO register
  return true;
}

bool handle_send(command_param_t param){
  // TODO
  printf("Send currently not supported.\n");
  return false;
}

static interface_handler_f interface_handlers[INTERFACE_COMMAND_COUNT-1] = {
  [SEARCHFOR] = handle_searchfor,
  [SEND] = handle_send,
  [FREQ] = handle_freq,
  [LIST] = handle_list,
  [GOTO] = handle_goto,
  [SPLIT] = handle_split,
  [ID] = handle_id};

bool handle_interface_command(interface_commands command, 
    command_param_t param){

  return interface_handlers[command](param);
}

bool strtol_check_error(long number){
  if (errno == ERANGE && number == LONG_MIN){
    printf ("underflow\n");
  }
  else if (errno == ERANGE && number == LONG_MAX){
    printf ("overflow\n");
  }
  else if (errno == EINVAL) { 
    printf ("base invalid\n");
  }
  else if (errno != 0 && number == 0) {
    printf ("unspecified error\n");
  } else {
    return true;
  }

  return false;
}

bool parse_goto (char* freq_str, frequency_t* freq) {
  long temp_freq;
  if (!strtol_check_error(temp_freq = strtol(freq_str, NULL, 10)))
    return false;
  
  if (temp_freq < 800 || 950 < temp_freq) 
    return false;

  *freq = temp_freq;
  return true;
}

bool parse_searchfor (char* to_find_str, routing_id_t* to_find){
  long temp_to_find;

  if (!strtol_check_error(
        temp_to_find = strtol(to_find_str, NULL, 16)))
    return false;

  if (temp_to_find > 0xFFFFFFFFFFFF)
    return false;

  memcpy(to_find->optional_MAC, (uint8_t*)&temp_to_find, 6);
  return true;
}

int input_to_array(char* input, char* args[MAX_ARGC]) {
  args[0] = input;
  int argc = 1;
  while (*input != '\0' && argc < MAX_ARGC) {
    if (*input == ' ') {
      args[argc] = input+1;
      argc++;
      *input = '\0';
    }
    input++;
    if (*input == '\n')
      *input = '\0';
  }
  return argc;
}

interface_commands get_command(char* command) {
  assert(command != NULL);

  if (strcmp(command, "searchfor") == 0) 
    return SEARCHFOR;

  if (strcmp(command, "send") == 0)
    return SEND;

  if (strcmp(command, "freq") == 0)
    return FREQ;

  if (strcmp(command, "list") == 0)
    return LIST;

  if (strcmp(command, "goto") == 0)
    return GOTO;

  if (strcmp(command, "split") == 0)
    return SPLIT;

  if (strcmp(command, "id") == 0)
    return ID;

  return UNKNOWN;
}

void wait_command_fetched(){
  for(;;){
    sleep_ms(1000);
    pthread_mutex_lock(&interface_lock);
    if (command.set) {
      pthread_mutex_unlock(&interface_lock);
      continue;
    }
    pthread_mutex_unlock(&interface_lock);
    return;
  }
}

void* interface_collect_user_input(void* arg) {
  
  printf("\nInsert command\n");

  char* args[MAX_ARGC];
  int argc;
  char* line = NULL;
  size_t n = 0;

  while (getline(&line, &n, stdin) != -1) {
    argc = input_to_array(line, args);
    if (argc <= 0 || MAX_ARGC < argc)
      continue;

    switch (get_command(args[0])) {
        case ID:
          pthread_mutex_lock(&interface_lock);
          command.set = true;
          command.type = ID;
          pthread_mutex_unlock(&interface_lock);
          wait_command_fetched();
          break;

        case LIST:
          pthread_mutex_lock(&interface_lock);
          command.set = true;
          command.type = LIST;
          pthread_mutex_unlock(&interface_lock);
          wait_command_fetched();
          break;

        case SPLIT:
          pthread_mutex_lock(&interface_lock);
          command.set = true;
          command.type = SPLIT;
          pthread_mutex_unlock(&interface_lock);
          wait_command_fetched();
          break;

        case FREQ:
          pthread_mutex_lock(&interface_lock);
          command.set = true;
          command.type = FREQ;
          pthread_mutex_unlock(&interface_lock);
          wait_command_fetched();
          break;

        case GOTO:{
          frequency_t destination_frequency;
          if (argc == 2 
              && parse_goto(args[1], &destination_frequency)) {
            pthread_mutex_lock(&interface_lock);
            command.set = true;
            command.type = GOTO;
            command.command_param.freq = destination_frequency;
            pthread_mutex_unlock(&interface_lock);
            wait_command_fetched();
          } else {
            printf("Invalid frequency input."
                " Valid frequencies are in range [800,950].\n");
          }
          break;}

        case SEND:
          printf("Send currently not supported.\n");
          break;

        case SEARCHFOR:{
          routing_id_t to_find;
          if (parse_searchfor(args[1], &to_find)) {

            pthread_mutex_lock(&interface_lock);
            command.set = true;
            command.type = SEARCHFOR;
            command.command_param.to_find = to_find;
            pthread_mutex_unlock(&interface_lock);
            wait_command_fetched();
          } else {
            printf("Invalid address input. \
                Try a 6 Byte MAC address.\n");
          }
          break;}

        default:
          printf("Unknown command. Commands: "
                 "id, goto, list, searchfor, freq, split, send\n");
    }
    printf("\nInsert command\n");
  }
  return NULL;
}

bool interface_do_action(){

  if (pthread_mutex_trylock(&interface_lock) != 0) {
    return false;
  }

  if (!command.set) {
    pthread_mutex_unlock(&interface_lock);
    return false;
  }

  handle_interface_command(command.type, command.command_param);
  command.set = false;
  pthread_mutex_unlock(&interface_lock);
  return true;
}

void interface_initialize(){
  pthread_mutex_init(&interface_lock, NULL);
  pthread_t user_input_thread;
  pthread_create(&user_input_thread, NULL, interface_collect_user_input, NULL);
}
