#include "src/protocol/message_handler.h"
#include "lib/datastructures/generic/generic_vector.h"
#include "lib/time_util.h"
#include "src/protocol/message.h"
#include "src/protocol/message_util.h"
#include "src/protocol/routing.h"
#include "src/protocol/search.h"
#include "src/protocol/swap.h"
#include "src/protocol/transfer.h"
#include "src/protocol/tree.h"
#include "src/state.h"
#include "src/transport.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

typedef bool (*handler_f)(message_t *msg);

bool handle_do_mute(message_t *msg);
bool handle_dont_mute(message_t *msg);

static handler_f message_handlers[MESSAGE_ACTION_COUNT][MESSAGE_TYPE_COUNT] = {
    /*[DO][MUTE] = handle_do_mute,*/
    /*[DONT][MUTE] = handle_dont_mute,*/
    [DO][SWAP] = handle_do_swap,
    [DO][TRANSFER] = handle_do_transfer,
    [WILL][TRANSFER] = handle_will_transfer,
    [DO][FIND] = handle_do_find,
    [DO][MIGRATE] = handle_do_migrate,
    [DO][SPLIT] = handle_do_split};

bool handle_do_mute(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == MUTE);

  if (!message_from_leader(msg)) {
    fprintf(stderr, "Received DO MUTE from non-leader, will not act on it.\n");
    return false;
  }

  gs.flags |= MUTED;

  return true;
}

bool handle_dont_mute(message_t *msg) {
  assert(message_action(msg) == DONT);
  assert(message_type(msg) == MUTE);

  if (!message_from_leader(msg)) {
    fprintf(stderr,
            "Received DONT MUTE from non-leader, will not act on it.\n");
    return false;
  }

  gs.flags &= ~MUTED;

  return true;
}

bool handle_message(message_t *msg) {
  assert(message_is_valid(msg));

  return message_handlers[message_action(msg)][message_type(msg)](msg);
}
