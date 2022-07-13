#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdbool.h>

void interface_initialize(void);
void* interface_collect_user_input(void*);
bool interface_do_action(void);

#endif // INTERFACE_H
