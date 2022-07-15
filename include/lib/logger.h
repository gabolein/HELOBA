#ifndef LOGGER_H
#define LOGGER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define PANIC_LEVEL 0
#define WARNING_LEVEL 1
#define DEBUG_LEVEL 2

#ifndef LOG_LEVEL
#define LOG_LEVEL DEBUG_LEVEL
#endif

#ifndef LOG_LABEL
#define LOG_LABEL "???"
#endif

#if LOG_LEVEL >= DEBUG_LEVEL
#define dbgln(...)                                                             \
  do {                                                                         \
    printf("(" LOG_LABEL ") ");                                                \
    printf(__VA_ARGS__);                                                       \
    printf("\n");                                                              \
  } while (0)
#else
#define dbgln(...)
#endif

#if LOG_LEVEL >= WARNING_LEVEL
#define warnln(...)                                                            \
  do {                                                                         \
    printf("[WARNING] (" LOG_LABEL ") ");                                      \
    printf(__VA_ARGS__);                                                       \
    printf("\n");                                                              \
  } while (0)
#else
#define warnln(...)
#endif

#if LOG_LEVEL >= PANIC_LEVEL
#define panicln(...)                                                           \
  do {                                                                         \
    printf("[PANIC] (" LOG_LABEL ") ");                                        \
    printf(__VA_ARGS__);                                                       \
    printf("\n");                                                              \
    exit(0);                                                                   \
  } while (0)
#else
#define panicln(...)
#endif

#endif
