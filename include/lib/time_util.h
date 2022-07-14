#ifndef TIME_UTIL_H
#define TIME_UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

void sleep_ms(uint32_t);
bool hit_timeout(size_t ms, struct timespec *start);
int timestamp_cmp(struct timespec, struct timespec); 
struct timespec timestamp_add_ms(struct timespec, uint32_t);

#endif // TIME_UTIL_H
