#define LOG_LEVEL WARNING_LEVEL
#define LOG_LABEL "Command"

#include "src/interface/command.h"
#include "lib/logger.h"
#include "src/protocol/message.h"
#include "src/protocol/message_formatter.h"
#include "src/protocol/search.h"
#include "src/protocol/transfer.h"
#include "src/state.h"
#include "src/transport.h"

bool handle_freq(__attribute__((unused)) command_param_t param) {
  printf("Current frequency: %u\n", gs.frequency);
  return true;
}

bool handle_list(__attribute__((unused)) command_param_t param) {
  if (!(gs.id.layer & leader)) {
    warnln("Node %s is not a leader and therefore has no list of nodes.",
           format_routing_id(gs.id));
    return false;
  }

  club_key_vector_t *keys = club_hashmap_keys(gs.members);
  for (unsigned i = 0; i < club_key_vector_size(keys); i++) {
    routing_id_t id = club_key_vector_at(keys, i);
    printf("%u. %s\n", i + 1, format_routing_id(id));
  }

  club_key_vector_destroy(keys);
  return true;
}

bool handle_id(__attribute__((unused)) command_param_t param) {
  printf("%s\n", format_routing_id(gs.id));
  return true;
}

bool handle_split(__attribute__((unused)) command_param_t param) {
  if (!(gs.id.layer & leader)) {
    warnln("Node %s is not a leader and therefore cannot perform split.",
           format_routing_id(gs.id));
    return false;
  }
  perform_split(SPLIT_DOWN);
  return true;
}

bool handle_goto(command_param_t param) {
  if (!perform_unregistration(param.freq)) {
    warnln("Couldn't unregister from current frequency.");
    return false;
  }

  transport_change_frequency(param.freq);
  return perform_registration(param.freq);
}

bool handle_searchfor(command_param_t param) {
  param.to_find.layer = specific;
  if (routing_id_equal(param.to_find, gs.id)) {
    warnln("Can't search for own id!");
    return false;
  }

  frequency_t found;
  if (!perform_search(param.to_find, &found)) {
    warnln("Couldn't find requested node %s", format_routing_id(param.to_find));
    return false;
  }

  command_param_t wrap = {.freq = found};
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
  // Wir verhindern, dass Leader andere Nodes suchen können, da sie unbedingt
  // auf ihrer Frequenz bleiben sollten.
  if (command == SEARCHFOR && gs.id.layer & leader) {
    warnln("Leaders cannot search for other nodes!");
    return false;
  }

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

  if (strcmp(command, "help") == 0)
    return HELP;

  return UNKNOWN;
}
