#define LOG_LEVEL WARNING_LEVEL
#define LOG_LABEL "Interface"

#include "lib/logger.h"
#include "src/config.h"
#include "src/protocol/message.h"
#include "src/protocol/message_formatter.h"
#include "src/protocol/tree.h"
#define _GNU_SOURCE
#include "lib/time_util.h"
#include "src/interface/command.h"
#include "src/interface/interface.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ARGC 3

interface_command_t command = {.set = false};
static pthread_mutex_t interface_lock;

bool strtol_check_error(long number) {
  if (errno == ERANGE && number == LONG_MIN) {
    warnln("underflow");
  } else if (errno == ERANGE && number == LONG_MAX) {
    warnln("overflow");
  } else if (errno == EINVAL) {
    warnln("base invalid");
  } else if (errno != 0 && number == 0) {
    warnln("unspecified error");
  } else {
    return true;
  }

  return false;
}

bool parse_goto(char *freq_str, frequency_t *freq) {
  errno = 0;
  long temp_freq = strtol(freq_str, NULL, 10);
  if (temp_freq == 0 && errno != 0) {
    warnln("Strtol error");
    return false;
  }

  if (temp_freq < FREQUENCY_BASE || FREQUENCY_CEILING < temp_freq) {
    return false;
  }

  *freq = temp_freq;
  return true;
}

bool parse_searchfor(char *to_find_str, routing_id_t *to_find) {
  if (sscanf(to_find_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &to_find->MAC[0],
             &to_find->MAC[1], &to_find->MAC[2], &to_find->MAC[3],
             &to_find->MAC[4], &to_find->MAC[5]) != 6)
    return false;

  return true;
}

int input_to_array(char *input, char *args[MAX_ARGC]) {
  args[0] = input;
  int argc = 1;
  while (*input != '\n' && argc < MAX_ARGC) {
    if (*input == ' ') {
      args[argc] = input + 1;
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

void wait_command_fetched() {
  bool not_fetched = true;
  while (not_fetched) {
    sleep_ms(300);
    pthread_mutex_lock(&interface_lock);
    not_fetched = command.set;
    pthread_mutex_unlock(&interface_lock);
  }
}

void print_help() {
  printf("=== General Commands\n"
         "help: Print this message\n"
         "id: Print own ID\n"
         "freq: Print current frequency\n"
         "goto <f>: Go to frequency f ∈ [%u, %u]\n"
         "searchfor <MAC>: Search for a Node with a given MAC address\n"
         "=== Leader Commands\n"
         "list: List all registered Nodes\n"
         "split: Execute SPLIT command\n"
         "quit: Take a guess\n",
         FREQUENCY_BASE, FREQUENCY_CEILING);
}

void print_prompt() { printf("\nf> "); }

void *interface_collect_user_input(void *arg) {
  print_prompt();

  char *args[MAX_ARGC];
  int argc;
  char *line = NULL;
  size_t n = 0;

  while (getline(&line, &n, stdin) != -1) {
    dbgln("Read line: %s", line);
    argc = input_to_array(line, args);
    if (argc <= 0 || MAX_ARGC < argc)
      continue;

    for (unsigned i = 0; i < argc; i++) {
      dbgln("args[%u] = %s", i, args[i]);
    }
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

    case SPLIT_NODES:
      pthread_mutex_lock(&interface_lock);
      command.set = true;
      command.type = SPLIT_NODES;
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

    case GOTO: {
      frequency_t destination_frequency;
      if (argc == 2 && parse_goto(args[1], &destination_frequency)) {
        pthread_mutex_lock(&interface_lock);
        command.set = true;
        command.type = GOTO;
        command.command_param.freq = destination_frequency;
        pthread_mutex_unlock(&interface_lock);
        wait_command_fetched();
      } else {
        printf("Invalid frequency input. Valid frequencies are ∈ [%u, %u].",
               FREQUENCY_BASE, FREQUENCY_CEILING);
      }
      break;
    }

    case SEND:
      printf("Send is currently not supported.");
      break;

    case SEARCHFOR: {
      routing_id_t to_find = routing_id_create(0, NULL);
      if (parse_searchfor(args[1], &to_find)) {
        to_find.layer = specific;
        dbgln("Got a SEARCHFOR command for %s", format_routing_id(to_find));
        pthread_mutex_lock(&interface_lock);
        command.set = true;
        command.type = SEARCHFOR;
        command.command_param.to_find = to_find;
        pthread_mutex_unlock(&interface_lock);
        wait_command_fetched();
      } else {
        printf("Invalid address input. Try a 6 Byte MAC address.");
      }
      break;
    }

    case HELP:
      print_help();
      break;

    default:
      printf("Unknown command \"%s\"\n", args[0]);
      print_help();
    }

    print_prompt();
  }
  free(line);
  return NULL;
}

bool interface_do_action() {
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

void interface_initialize() {
  pthread_mutex_init(&interface_lock, NULL);
  pthread_t user_input_thread;
  pthread_create(&user_input_thread, NULL, interface_collect_user_input, NULL);
  print_help();
}
