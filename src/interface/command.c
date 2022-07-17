#define LOG_LEVEL DEBUG_LEVEL
#define LOG_LABEL "Command"

#include "src/interface/command.h"
#include "lib/logger.h"
#include "src/protocol/search.h"
#include "src/protocol/transfer.h"
#include "src/state.h"
#include "src/transport.h"

bool handle_freq(__attribute__((unused)) command_param_t param) {
  dbgln("Current frequency: %u", gs.frequencies.current);
  return true;
}

void print_id(uint8_t MAC[6]) {
  for (unsigned i = 0; i < MAC_SIZE; i++) {
    if (i != 0) {
      printf(":");
    }

    printf("%hhx", MAC[i]);
  }
}

bool handle_list(__attribute__((unused)) command_param_t param) {
  if (!(gs.id.layer & leader)) {
    dbgln("Node is not a leader and therefore has no list of nodes.");
    return false;
  }

  routing_id_t_vector_t *keys = club_hashmap_keys(gs.members);
  unsigned nkeys = routing_id_t_vector_size(keys);
  for (size_t i = 0; i < nkeys; i++) {
    routing_id_t id = routing_id_t_vector_at(keys, i);
    printf("1. ");
    print_id(id.MAC);
  }

  routing_id_t_vector_destroy(keys);
  return true;
}

bool handle_id(__attribute__((unused)) command_param_t param) {
  print_id(gs.id.MAC);

  return true;
}

bool handle_split(__attribute__((unused)) command_param_t param) {
  if (!(gs.id.layer & leader)) {
    dbgln("Node is not a leader and therefore cannot perform split.");
    return false;
  }
  perform_split();
  return true;
}

bool handle_goto(command_param_t param) {
  if (!perform_unregistration(param.freq)) {
    warnln("Couldn't unregister from current frequency.");
    return false;
  }

  transport_change_frequency(param.freq);
  return perform_registration();
}

bool handle_searchfor(command_param_t param) {
  param.to_find.layer = specific;
  if (!perform_search(param.to_find)) {
    warnln("Couldn't find requested node");
    return false;
  }

  dbgln("Found requested node, is on frequency %u", gs.frequencies.current);
  transport_change_frequency(gs.frequencies.previous);

  // NOTE: previous ist richtig, weil transport_change_frequency() automatisch
  // previous und current anpasst
  command_param_t wrap = {.freq = gs.frequencies.previous};
  return handle_goto(wrap);
}

bool handle_send(command_param_t param) {
  (void)param;
  warnln("Send currently not supported.");
  return false;
}

static interface_handler_f interface_handlers[INTERFACE_COMMAND_COUNT - 1] = {
    [SEARCHFOR] = handle_searchfor,
    [SEND] = handle_send,
    [FREQ] = handle_freq,
    [LIST] = handle_list,
    [GOTO] = handle_goto,
    [SPLIT_NODES] = handle_split,
    [ID] = handle_id};

bool handle_interface_command(interface_commands command,
                              command_param_t param) {

  return interface_handlers[command](param);
}

interface_commands get_command(char *command) {
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
    return SPLIT_NODES;

  if (strcmp(command, "id") == 0)
    return ID;

  return UNKNOWN;
}
