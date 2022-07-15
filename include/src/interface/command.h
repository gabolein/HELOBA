#ifndef COMMAND_H
#define COMMAND_H

#include <stdbool.h>
#include "src/protocol/message.h"
#include "src/protocol/routing.h"

#define INTERFACE_COMMAND_COUNT 8

typedef enum {
  SEARCHFOR,
  SEND,
  FREQ,
  LIST,
  GOTO,
  SPLIT_NODES,
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

typedef bool (*interface_handler_f)(command_param_t param);

interface_commands get_command(char*);
bool handle_interface_command(interface_commands, command_param_t);

#endif
