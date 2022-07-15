#include "src/interface/command.h"
#include "src/transport.h"
#include "src/state.h"
#include "src/protocol/routing.h"

bool handle_freq(
    __attribute__ ((unused))command_param_t param){
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
  printf("Node ID: ");

  for (size_t i = 0; i < 6; i++) {
    printf("%x", gs.id.optional_MAC[i]);

    if (i != 5)
      putchar(':');
  }

  printf("\n");
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
  param.to_find.layer = specific;
  return search_for(param.to_find);
}

bool handle_goto(command_param_t param){
  message_t unregister = message_create(WILL, TRANSFER);
  unregister.payload.transfer = (transfer_payload_t){.to = param.freq};
  routing_id_t receiver = {.layer = leader};
  transport_send_message(&unregister, receiver);

  transport_change_frequency(param.freq);
  // TODO election algorithm
  message_t join_request = message_create(WILL, TRANSFER);
  join_request.payload.transfer = (transfer_payload_t){.to = param.freq};

  transport_send_message(&join_request, receiver);
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

