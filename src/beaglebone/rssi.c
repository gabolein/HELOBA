#define LOG_LEVEL DEBUG_LEVEL
#define LOG_LABEL "RSSI"

#include "src/beaglebone/rssi.h"
#include "lib/logger.h"
#include "lib/time_util.h"
#include "src/beaglebone/registers.h"
#include <SPIv1.h>
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define PREAMBLE_DETECTION (1 << 7)

static int8_t global_rssi_threshold = 0;
static bool global_rssi_threshold_valid = false;

bool detect_RSSI(unsigned timeout_ms) {
  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
  while (!hit_timeout(timeout_ms, &start)) {
    int8_t curr_rssi;
    if (read_RSSI(&curr_rssi) && curr_rssi >= global_rssi_threshold)
      return true;
  }
  return false;
}

bool read_RSSI(int8_t *out) {
  int reg;
  cc1200_reg_read(RSSI1, &reg);

  if (reg == -128)
    return false;

  *out = reg;
  return true;
}

void start_receiver_blocking() {
  cc1200_cmd(SRX);

  do {
    cc1200_cmd(SNOP);
  } while (get_status_cc1200() != 1);
}

void enable_preamble_detection() {
  int preamble_reg;
  cc1200_reg_read(PREAMBLE_CFG0, &preamble_reg);
  cc1200_reg_write(PREAMBLE_CFG0, preamble_reg | PREAMBLE_DETECTION);
}

void disable_preamble_detection() {
  int preamble_reg;
  cc1200_reg_read(PREAMBLE_CFG0, &preamble_reg);
  cc1200_reg_write(PREAMBLE_CFG0, preamble_reg & ~PREAMBLE_DETECTION);
}

bool calculate_RSSI_threshold(int8_t *out) {
  int8_t rssi = 0;
  size_t samples = 0;
  size_t errors = 0;

  float threshold = 0;
  float f_rssi = 0;

  start_receiver_blocking();

  struct timespec start_time;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);

  while (!hit_timeout(RSSI_CONFIG_DURATION_MS, &start_time)) {
    if (!read_RSSI(&rssi)) {
      errors++;
      continue;
    }

    f_rssi = (float)rssi;
    threshold = threshold + (f_rssi - threshold) / ++samples;
  }

  dbgln("Collected %lu samples, got %lu errors", samples, errors);

  if (errors > samples)
    return false;

  assert(threshold >= INT8_MIN && threshold <= INT8_MAX);

  *out = (int8_t)round(threshold);
  if (__builtin_add_overflow(*out, RSSI_DELTA, out))
    return false;

  return true;
}

void set_rssi_threshold(int8_t threshold) {
  global_rssi_threshold = threshold;
  global_rssi_threshold_valid = true;
}

int8_t get_rssi_threshold() {
  assert(global_rssi_threshold_valid);
  return global_rssi_threshold;
}
