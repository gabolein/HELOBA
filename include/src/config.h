#ifndef CONFIG_H
#define CONFIG_H

#define FREQUENCY_BASE 820
#define FREQUENCY_CEILING 960

// FIXME: Wir sollten uns für Timeouts auf eine Einheit festlegen und dann
// #defines für jede Einheit bereitstellen, mit der zu dieser Einheit
// konvertiert wird. Viele von diesen Timeouts sind viel zu groß und könnten
// wahrscheinlich auch im Mikrosekundenbereich liegen. Jedenfalls müssen wir uns
// mit diesem Ansatz nicht auf eine Einheit festlegen.
#define FIND_RESPONSE_TIMEOUT_MS 10
#define SWAP_RESPONSE_TIMEOUT_MS 10
#define TRANSFER_RESPONSE_TIMEOUT_MS 50

#define MIGRATION_BLOCK_DURATION_MS 10
#define SWAP_BLOCK_DURATION_MS 50

#if !defined(VIRTUAL)

#define RSSI_PADDING 20
#define RADIO_FIRST_BYTE_WAIT_TIME_MS 1000
#define RADIO_NEXT_BYTE_WAIT_TIME_MS 1000
#define RADIO_BACKOFF_TIMEOUT_MS 300 // 8 (?) backoffs + buffer

#endif

#endif
