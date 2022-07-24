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
#define log__impl(slug, fmt, ...)
#else
#define log__impl(slug, fmt, ...)                                              \
  fprintf(stderr, slug " " fmt "%s", __VA_ARGS__);
#endif

// TODO: Variadic Function schreiben, dann können wir auch Sachen wie PID etc
// vor den Log String schreiben.
#if LOG_LEVEL >= DEBUG_LEVEL
#define dbgln(...) log__impl("(" LOG_LABEL ")", __VA_ARGS__, "\n")
#else
#define dbgln(...)
#endif

#if LOG_LEVEL >= WARNING_LEVEL
#define warnln(...)                                                            \
  log__impl("\x1b[33;4m[WARNING]\x1b[0m (" LOG_LABEL ")", __VA_ARGS__, "\n")
#else
#define warnln(...)
#endif

#if LOG_LEVEL >= PANIC_LEVEL
#define panicln(...)                                                           \
  do {                                                                         \
    log__impl("\x1b[31;4m[PANIC]\x1b[0m (" LOG_LABEL ")", __VA_ARGS__, "\n");  \
    exit(0);                                                                   \
  } while (0)
#else
#define panicln(...)
#endif

#endif