#include "src/beaglebone/frequency.h"
#include "src/beaglebone/backoff.h"
#include "src/beaglebone/registers.h"
#include <SPIv1.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define CRYSTAL_FREQUENCY (40 * MHz)
#define FREQ_OFF 0

uint8_t read_LO_divider_value() {
  int reg;
  cc1200_reg_read(FS_CFG, &reg);

  switch (reg & 0x0F) {
  case 0b0010:
    return 4; // 820.0 - 960.0 MHz band
  case 0b0100:
    return 8; // 410.0 - 480.0 MHz band
  case 0b0110:
    return 12; // 273.3 - 320.0 MHz band
  case 0b1000:
    return 16; // 205.0 - 240.0 MHz band
  case 0b1010:
    return 20; // 164.0 - 192.0 MHz band
  case 0b1011:
    return 24; // 136.7 - 160.0 MHz band
  default:
    // NOTE: Im Users Guide S. 91 steht, dass alle anderen Werte "Not In Use"
    // sind. Heißt das, dass in diesen Fällen keine Frequenz festgelegt werden
    // kann, oder sollte LO Divider hier auf 1 gesetzt werden?
    // FIXME: assert() wird in release-builds mit NDEBUG rauskompiliert, Weg
    // finden das zu verhindern
    assert(false);
  }
}

int write_LO_divider_value(uint8_t value) {
  switch (value) {
  case 4:
    return cc1200_reg_write(FS_CFG, 0b0010);
  case 8:
    return cc1200_reg_write(FS_CFG, 0b0100);
  case 12:
    return cc1200_reg_write(FS_CFG, 0b0110);
  case 16:
    return cc1200_reg_write(FS_CFG, 0b1000);
  case 20:
    return cc1200_reg_write(FS_CFG, 0b1010);
  case 24:
    return cc1200_reg_write(FS_CFG, 0b1011);
  default:
    assert(false);
  }
}

uint8_t determine_LO_divider_value_from_frequency(uint32_t hr_freq) {
  uint32_t freq_MHz = hr_freq / MHz;

  if (freq_MHz >= 820 && freq_MHz <= 960) {
    return 4;
  } else if (freq_MHz >= 410 && freq_MHz <= 480) {
    return 8;
    // NOTE: ich dachte, Frequenzen sollen keine Komma-Werte haben, warum
    // startet dieses Frequenzband also bei 273.3?
  } else if (freq_MHz >= 273 && freq_MHz <= 320) {
    return 12;
  } else if (freq_MHz >= 205 && freq_MHz <= 240) {
    return 16;
  } else if (freq_MHz >= 164 && freq_MHz <= 192) {
    return 20;
  } else if (freq_MHz >= 136 && freq_MHz <= 160) {
    return 24;
  }

  assert(false);
}

void write_hardware_frequency_to_register(uint32_t freq) {
  cc1200_reg_write(FREQ0, (freq & 0x000000FF) >> 0);
  cc1200_reg_write(FREQ1, (freq & 0x0000FF00) >> 8);
  cc1200_reg_write(FREQ2, (freq & 0x00FF0000) >> 16);
}

uint32_t read_hardware_frequency_from_register() {
  int freq_parts[3];

  cc1200_reg_read(FREQ0, &freq_parts[0]);
  cc1200_reg_read(FREQ1, &freq_parts[1]);
  cc1200_reg_read(FREQ2, &freq_parts[2]);

  uint32_t hw_freq =
      (freq_parts[2] << 16) | (freq_parts[1] << 8) | (freq_parts[0] << 0);

  return hw_freq;
}

// NOTE: soll freq in Hz oder in MHz sein?
// Am besten ohne MHz, der aufrufende Code weiß ja, was damit gemeint ist.
void set_transceiver_frequency(uint32_t freq) {
  // NOTE: is es besser, einfach zu crashen oder sollten wir stattdessen einen
  // Fehlercode zurückgeben?
  // NOTE: sollen wir überhaupt annehmen, dass die Frequenz immer in ganzen MHz
  // ist? Ein paar Frequenzbänder beginnen bei Kommazahlen...
  assert(freq % MHz == 0);

  // Radio erst in IDLE Mode setzen, bevor Registerwerte geändert werden (siehe
  // User Guide Section 9.13)
  cc1200_cmd(SIDLE);

  uint8_t lo_divider = determine_LO_divider_value_from_frequency(freq);
  write_LO_divider_value(lo_divider);

  // NOTE: für originale Formel siehe User Guide Section 9.12
  uint32_t hw_freq =
      ((uint64_t)freq * lo_divider << 16) / CRYSTAL_FREQUENCY - (FREQ_OFF >> 2);
  write_hardware_frequency_to_register(hw_freq);
}

void print_transceiver_frequency() {
  uint32_t hw_freq = read_hardware_frequency_from_register();
  uint32_t freq = ((hw_freq >> 16) + (FREQ_OFF >> 18)) * CRYSTAL_FREQUENCY /
                  read_LO_divider_value();

  printf("[INFO] Current Transceiver Frequency: %uMHz\n", freq / MHz);
}
