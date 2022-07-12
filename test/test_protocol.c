#include "lib/datastructures/u8_vector.h"
#include "src/protocol/message.h"
#include "src/protocol/message_parser.h"
#include "src/protocol/routing.h"
#include <criterion/criterion.h>
#include <criterion/internal/assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

routing_id_t get_random_id() {
  routing_id_t id;
  // NOTE: können nochmal schauen, ob das auch random gemacht werden soll
  id.layer = specific;

  int fd = open("/dev/urandom", O_RDONLY);
  if (fd < 0) {
    perror("open");
    exit(EXIT_FAILURE);
  }

  if (read(fd, id.optional_MAC, sizeof(id.optional_MAC)) < 0) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  return id;
}

message_t create_basic_test_message(message_action_t action,
                                    message_type_t type) {
  message_t msg;
  msg.header.action = action;
  msg.header.type = type;
  msg.header.sender_id = get_random_id();
  msg.header.receiver_id = get_random_id();

  memset(&msg.payload, 0, sizeof(msg.payload));

  return msg;
}

bool id_equal(routing_id_t id1, routing_id_t id2) {
  return id1.layer == id2.layer &&
         memcmp(id1.optional_MAC, id2.optional_MAC, MAC_SIZE) == 0;
}

bool local_tree_equal(local_tree_t t1, local_tree_t t2) {
  if (t1.opt != t2.opt) {
    return false;
  }

  if ((t1.opt & OPT_SELF) && t1.self != t2.self)
    return false;

  if ((t1.opt & OPT_PARENT) && t1.parent != t2.parent)
    return false;

  if ((t1.opt & OPT_LHS) && t1.lhs != t2.lhs)
    return false;

  if ((t1.opt & OPT_RHS) && t1.rhs != t2.rhs)
    return false;

  return true;
}

bool message_equal(message_t *m1, message_t *m2) {
  if (m1->header.action != m2->header.action ||
      m1->header.type != m2->header.type ||
      !id_equal(m1->header.receiver_id, m2->header.receiver_id) ||
      !id_equal(m1->header.sender_id, m2->header.sender_id)) {
    return false;
  }

  switch (m1->header.type) {
  case FIND:
    return id_equal(m1->payload.find.to_find, m2->payload.find.to_find);
  case UPDATE:
    return m1->payload.update.old == m2->payload.update.old &&
           m1->payload.update.updated == m2->payload.update.updated;
  case SWAP:
    return m1->payload.swap.activity_score == m2->payload.swap.activity_score &&
           local_tree_equal(m1->payload.swap.tree, m2->payload.swap.tree);
  case TRANSFER:
    return m1->payload.transfer.to == m2->payload.transfer.to;
  default:
    return true;
  }
}

Test(protocol, find_message) {
  message_t msg = create_basic_test_message(DO, FIND);
  msg.payload.find = (find_payload_t){
      .to_find = get_random_id(),
  };

  u8_vector_t *v = u8_vector_create();
  pack_message(v, &msg);
  message_t unpacked = unpack_message(v->data, u8_vector_size(v));

  cr_assert(message_equal(&msg, &unpacked));
}

Test(protocol, update_message) {
  message_t msg = create_basic_test_message(DO, UPDATE);
  msg.payload.update = (update_payload_t){
      .old = 1337,
      .updated = 9001,
  };

  u8_vector_t *v = u8_vector_create();
  pack_message(v, &msg);
  message_t unpacked = unpack_message(v->data, u8_vector_size(v));

  cr_assert(message_equal(&msg, &unpacked));
}

Test(protocol, swap_message) {
  message_t msg = create_basic_test_message(WILL, SWAP);
  msg.payload.swap = (swap_payload_t){
      .activity_score = 13,
      .tree =
          {
              .opt = OPT_SELF | OPT_PARENT | OPT_LHS,
              .self = 345,
              .parent = 177,
              .lhs = 45,
          },
  };

  u8_vector_t *v = u8_vector_create();
  pack_message(v, &msg);
  message_t unpacked = unpack_message(v->data, u8_vector_size(v));

  cr_assert(message_equal(&msg, &unpacked));
}

Test(protocol, transfer_message) {
  message_t msg = create_basic_test_message(DO, TRANSFER);
  msg.payload.transfer = (transfer_payload_t){
      .to = 147,
  };

  u8_vector_t *v = u8_vector_create();
  pack_message(v, &msg);
  message_t unpacked = unpack_message(v->data, u8_vector_size(v));

  cr_assert(message_equal(&msg, &unpacked));
}

Test(protocol, mute_message) {
  message_t msg = create_basic_test_message(DONT, MUTE);

  u8_vector_t *v = u8_vector_create();
  pack_message(v, &msg);
  message_t unpacked = unpack_message(v->data, u8_vector_size(v));

  cr_assert(message_equal(&msg, &unpacked));
}