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

  if (read(fd, id.MAC, sizeof(id.MAC)) < 0) {
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
  return id1.layer == id2.layer && memcmp(id1.MAC, id2.MAC, MAC_SIZE) == 0;
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
  case SWAP:
    return m1->payload.swap.score == m2->payload.swap.score &&
           m1->payload.swap.with == m2->payload.swap.with;
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

Test(protocol, swap_message) {
  message_t msg = create_basic_test_message(WILL, SWAP);
  msg.payload.swap = (swap_payload_t){
      .score = 13,
      .with = 865,
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