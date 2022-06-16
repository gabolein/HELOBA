#ifndef RSSI_H
#define RSSI_H

#include <stdbool.h>
#include <stdint.h>

#define RSSI_CONFIG_DURATION_MS 5000
#define RSSI_DELTA 10
#define RSSI_INVALID -128

void start_receiver_blocking();
void enable_preamble_detection();
void disable_preamble_detection();

bool read_RSSI(int8_t *);
bool calculate_RSSI_threshold(int8_t *);
void set_rssi_threshold(int8_t threshold);
int8_t get_rssi_threshold();

#endif
