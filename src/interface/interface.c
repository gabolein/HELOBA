#define _GNU_SOURCE
#include "src/interface/command.h"
#include "src/interface/interface.h"
#include "lib/time_util.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

#define MAX_ARGC 3

interface_command_t command = {.set = false};
static pthread_mutex_t interface_lock;

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
  if (!strtol_check_error(
        temp_freq = strtol(freq_str, NULL, 10)))
    return false;
  
  if (temp_freq < 800 || 950 < temp_freq) 
    return false;

  *freq = temp_freq;
  return true;
}

bool parse_searchfor (char* to_find_str, routing_id_t* to_find) {

  if (sscanf(to_find_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
        &to_find->optional_MAC[0], 
        &to_find->optional_MAC[1], 
        &to_find->optional_MAC[2], 
        &to_find->optional_MAC[3], 
        &to_find->optional_MAC[4], 
        &to_find->optional_MAC[5]) != 6)
    return false;

  return true;
}

int input_to_array(char* input, char* args[MAX_ARGC]) {
  args[0] = input;
  int argc = 1;
  while (*input != '\n' && argc < MAX_ARGC) {
    if (*input == ' ') {
      args[argc] = input+1;
      argc++;
      *input = '\0';
    }
    input++;
  }
  // getline does not sever linefeed
  if (*input == '\n')
  *input = '\0';
  return argc;
}

void wait_command_fetched(){
  bool not_fetched = true;
  while (not_fetched) {
    sleep_ms(300);
    pthread_mutex_lock(&interface_lock);
    not_fetched = command.set;
    pthread_mutex_unlock(&interface_lock);
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

    // TODO refactor
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
          routing_id_t to_find = {0, {0}};
          if (parse_searchfor(args[1], &to_find)) {

            pthread_mutex_lock(&interface_lock);
            command.set = true;
            command.type = SEARCHFOR;
            command.command_param.to_find = to_find;
            pthread_mutex_unlock(&interface_lock);
            wait_command_fetched();
          } else {
            printf("Invalid address input." 
                " Try a 6 Byte MAC address.\n");
          }
          break;}

        default:
          printf("Unknown command. Commands: "
                 "id, goto, list, searchfor, freq, split, send\n");
    }
    printf("\nInsert command\n");
  }
  free(line);
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
