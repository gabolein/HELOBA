#ifndef FREQUENCY_H
#define FREQUENCY_H

#include <stdint.h>

#define MHz 1000000

void set_transceiver_frequency(uint32_t freq);
void print_transceiver_frequency();

#endif
