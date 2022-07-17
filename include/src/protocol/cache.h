#ifndef CACHE_H
#define CACHE_H

#include "src/protocol/message.h"
#include <stdbool.h>
#include <time.h>

// Verarbeiteter Cache Eintrag, der als Antwort auf DO FIND an andere Nodes
// geschickt werden kann.
//
// f: Die Frequenz, auf der der Node mit der angefragten ID sein könnte
// timedelta: Alter des Cacheeintrags in Mikrosekunden. Damit ist das maximale
// Alter eines Cache Eintrags ~70 Minuten.
typedef struct {
  frequency_t f;
  unsigned timedelta_us;
} cache_hint_t;

bool cache_hit(routing_id_t id);
cache_hint_t cache_get(routing_id_t id);
void cache_remove(routing_id_t id);
void cache_insert(routing_id_t id, frequency_t f);

#endif