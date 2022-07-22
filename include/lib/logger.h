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

#if defined(NDEBUG)

#define dbgln(...)
#define warnln(...)
#define panicln(...) exit(0)

#else

#if LOG_LEVEL >= DEBUG_LEVEL
#define dbgln(...)                                                             \
  do {                                                                         \
    fprintf(stderr, "(" LOG_LABEL ") ");                                       \
    fprintf(stderr, __VA_ARGS__);                                              \
    fprintf(stderr, "\n");                                                     \
  } while (0)
#else
#define dbgln(...)
#endif

#if LOG_LEVEL >= WARNING_LEVEL
#define warnln(...)                                                            \
  do {                                                                         \
    fprintf(stderr, "\x1b[33;4m[WARNING]\x1b[0m (" LOG_LABEL ") ");            \
    fprintf(stderr, __VA_ARGS__);                                              \
    fprintf(stderr, "\n");                                                     \
  } while (0)
#else
#define warnln(...)
#endif

#if LOG_LEVEL >= PANIC_LEVEL
#define panicln(...)                                                           \
  do {                                                                         \
    fprintf(stderr, "\x1b[31;4m[PANIC]\x1b[0m (" LOG_LABEL ") ");              \
    fprintf(stderr, __VA_ARGS__);                                              \
    fprintf(stderr, "\n");                                                     \
    exit(0);                                                                   \
  } while (0)
#else
#define panicln(...)
#endif

#endif

#endif