#ifndef TIME_UTIL_H
#define TIME_UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

void sleep_ms(uint32_t);
bool hit_timeout(size_t ms, struct timespec *start);

#endif // TIME_UTIL_H
