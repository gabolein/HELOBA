#ifndef MESSAGE_PARSER_H
#define MESSAGE_PARSER_H

// TODO: Parser als Name ist glaube ich nicht ganz richtig. Vielleicht einen
// besseren Namen finden?

#include "lib/datastructures/u8_vector.h"
#include "src/protocol/message.h"

void pack_message(u8_vector_t *v, message_t *msg);
message_t unpack_message(uint8_t *buffer, unsigned length);

#endif