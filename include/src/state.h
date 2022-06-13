#ifndef STATE_H
#define STATE_H

#include <stdint.h>

typedef struct {
  uint8_t virtid;
  uint16_t frequency;
} user_state;

user_state state;

#endif