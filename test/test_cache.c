#include "src/protocol/cache.h"
#include "src/protocol/message.h"
#include <criterion/criterion.h>
#include <criterion/internal/assert.h>
#include <unistd.h>

void setup_cache() {
  cache_initialize();
}

void teardown_cache() {
  cache_teardown();
}

Test(cache, insert_frequency, .init=setup_cache, .fini=teardown_cache) {
  routing_id_t cache_id;
  memset(&cache_id, 0, sizeof(cache_id));
  cache_id.layer = specific;
  uint8_t MAC[6] = { 42, 42, 42, 42, 42, 42};
  memcpy(cache_id.MAC, MAC, sizeof(MAC));
  cache_insert(cache_id, 800);

  /*cr_assert(cache_hit(cache_id));*/
  /*cr_assert(cache_get(cache_id).f == 800);*/
}

/*Test(cache, insert_timestamp) {*/
  /*cache_initialize();*/
  /*routing_id_t cache_id;*/
  /*memset(&cache_id, 0, sizeof(cache_id));*/
  /*cache_id.layer = specific;*/
  /*uint8_t MAC[6] = { 42, 42, 42, 42, 42, 42};*/
  /*memcpy(cache_id.MAC, MAC, sizeof(MAC));*/
  /*cache_insert(cache_id, 800);*/

  /*sleep(3);*/

  /*cr_assert(cache_hit(cache_id));*/
  /*cache_hint_t hint = cache_get(cache_id); */
  /*cr_assert(2000000 < hint.timedelta_us */
      /*&& hint.timedelta_us < 4000000);*/

  /*cache_teardown();*/
/*}*/

/*Test(cache, insert_max) {*/

  /*cache_initialize();*/
  /*routing_id_t cache_id;*/
  /*memset(&cache_id, 0, sizeof(cache_id));*/
  /*cache_id.layer = specific;*/
  /*for (unsigned i = 0; i < 32; i++) {*/
    /*uint8_t MAC[6] = { (uint8_t)i, 42, 42, 42, 42, 42};*/
    /*memcpy(cache_id.MAC, MAC, sizeof(MAC));*/
    /*cache_insert(cache_id, 800+i);*/
  /*}*/

  /*for (unsigned i = 0; i < 32; i++) {*/
    /*uint8_t MAC[6] = { (uint8_t)i, 42, 42, 42, 42, 42};*/

    /*memcpy(cache_id.MAC, MAC, sizeof(MAC));*/
    /*cr_assert(cache_hit(cache_id));*/
    /*cr_assert(cache_get(cache_id).f == 800+i);*/
  /*}*/

  /*cache_teardown();*/
/*}*/

/*Test(cache, insert_replace) {*/
  /*cache_initialize();*/
  /*routing_id_t cache_id;*/
  /*memset(&cache_id, 0, sizeof(cache_id));*/
  /*cache_id.layer = specific;*/

  /*for (unsigned i = 0; i < 33; i++) {*/
    /*uint8_t MAC[6] = { (uint8_t)i, 42, 42, 42, 42, 42};*/
    /*memcpy(cache_id.MAC, MAC, sizeof(MAC));*/
    /*cache_insert(cache_id, 800+i);*/
  /*}*/

  /*for (unsigned i = 0; i < 33; i++) {*/
    /*uint8_t MAC[6] = { (uint8_t)i, 42, 42, 42, 42, 42};*/

    /*memcpy(cache_id.MAC, MAC, sizeof(MAC));*/
    /*if (i == 0) {*/
      /*cr_assert(!cache_hit(cache_id));*/
    /*} else {*/
      /*cr_assert(cache_hit(cache_id));*/
      /*cr_assert(cache_get(cache_id).f == 800+i);*/
    /*}*/
  /*}*/

  /*cache_teardown();*/
/*}*/

/*Test(cache, insert_renew_timeout) {*/
  /*cache_initialize();*/
  /*routing_id_t cache_id;*/
  /*memset(&cache_id, 0, sizeof(cache_id));*/
  /*cache_id.layer = specific;*/

  /*routing_id_t renewed_id;*/
  /*routing_id_t gone_id;*/
  /*for (unsigned i = 0; i < 33; i++) {*/
    /*uint8_t MAC[6] = { (uint8_t)i, 42, 42, 42, 42, 42};*/
    /*memcpy(cache_id.MAC, MAC, sizeof(MAC));*/

    /*if (i == 0) {*/
      /*memcpy(&renewed_id, &cache_id, sizeof(cache_id));*/
    /*}*/
    
    /*if (i == 1) {*/
      /*memcpy(&gone_id, &cache_id, sizeof(cache_id));*/
    /*}*/

    /*if (i == 32) {*/
      /*cr_assert(cache_hit(renewed_id));*/
    /*}*/

    /*cache_insert(cache_id, 800+i);*/
  /*}*/

  /*for (unsigned i = 0; i < 33; i++) {*/
    /*uint8_t MAC[6] = { (uint8_t)i, 42, 42, 42, 42, 42};*/

    /*memcpy(cache_id.MAC, MAC, sizeof(MAC));*/
    /*if (i == 1) {*/
      /*cr_assert(!cache_hit(cache_id));*/
    /*} else {*/
      /*cr_assert(cache_hit(cache_id));*/
      /*cr_assert(cache_get(cache_id).f == 800+i);*/
    /*}*/
  /*}*/

  /*cache_teardown();*/
/*}*/
