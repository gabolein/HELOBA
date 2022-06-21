#ifndef MESSAGE_H
#define MESSAGE_H

typedef enum {
  DO,
  DONT,
  WILL,
  WONT
} message_action;

typedef enum {
  REPORT,
  FIND,
  UPDATE,
  SWAP,
  TRANSFER,
  MUTE
} message_type;

#endif
