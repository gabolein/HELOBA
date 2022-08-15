#ifndef CONFIG_H
#define CONFIG_H

#define FREQUENCY_BASE 820
#define FREQUENCY_CEILING 850

// FIXME: Wir sollten uns für Timeouts auf eine Einheit festlegen und dann
// #defines für jede Einheit bereitstellen, mit der zu dieser Einheit
// konvertiert wird. Viele von diesen Timeouts sind viel zu groß und könnten
// wahrscheinlich auch im Mikrosekundenbereich liegen. Jedenfalls müssen wir uns
// mit diesem Ansatz nicht auf eine Einheit festlegen.
#define FIND_RESPONSE_TIMEOUT_MS 50
#define SWAP_RESPONSE_TIMEOUT_MS 50
#define TRANSFER_RESPONSE_TIMEOUT_MS 50

#define MIGRATION_BLOCK_DURATION_MS 10
#define SWAP_BLOCK_DURATION_MS 5000
#define MIN_SPLIT_SCORE 5

// NOTE: Es wäre interessant, die aktuelle Cachegröße abhängig vom Alter des
// letzten Eintrags zu machen. Diese statische Größe sollte aber erstmal
// ausreichen.
#define CACHE_SIZE 8

#if !defined(VIRTUAL)

// Radio Settings: Finetuned, stuff will DEFINITELY break if lowered
#define RSSI_PADDING 50
#define RADIO_FIRST_BYTE_WAIT_TIME_MS 500
#define RADIO_NEXT_BYTE_WAIT_TIME_MS 200
#define FREQUENCY_SLEEP_TIME_MS 1000
#define RADIO_RECV_BLOCK_WAIT_TIME_MS 1000

#define MAX_RECEPTION_DURATION                                                 \
  RADIO_FIRST_BYTE_WAIT_TIME_MS +                                              \
      RADIO_NEXT_BYTE_WAIT_TIME_MS * 50 // All our packets are smaller than 50
#define RADIO_BACKOFF_TIMEOUT_MS MAX_RECEPTION_DURATION * 8

#endif

#endif
