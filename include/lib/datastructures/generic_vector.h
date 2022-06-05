#ifndef GENERIC_VECTOR_H
#define GENERIC_VECTOR_H

// 　　　 _ _　 ξ
//　　　 (´ 　 ｀ヽ、  　　  __           This will be you after reading
//　　⊂,_と（　 　 ）⊃　 （__(）､;.o：。   this code. Turn back now!!!
//　　　　　　Ｖ　Ｖ　　　　　　 　 　 ﾟ*･:.｡

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAKE_SPECIFIC_VECTOR_HEADER(T, name)                                   \
  typedef struct {                                                             \
    unsigned capacity;                                                         \
    unsigned size;                                                             \
    T *data;                                                                   \
  } name##_vector_t;                                                           \
                                                                               \
  name##_vector_t *name##_vector_create();                                     \
  name##_vector_t *name##_vector_create_with_capacity(unsigned capacity);      \
  unsigned name##_vector_size(name##_vector_t *v);                             \
  T name##_vector_at(name##_vector_t *v, unsigned index);                      \
  void name##_vector_append(name##_vector_t *v, T item);                       \
  void name##_vector_swap(name##_vector_t *v, unsigned i1, unsigned i2);       \
  T name##_vector_remove(name##_vector_t *v, unsigned index);                  \
  void name##_vector_destroy(name##_vector_t *v);

#define MAKE_SPECIFIC_VECTOR_SOURCE(T, name)                                   \
  void __name##_v_sanity_check(name##_vector_t *v);                            \
  void __name##_v_expand(name##_vector_t *v);                                  \
                                                                               \
  void __name##_v_sanity_check(name##_vector_t *v) {                           \
    assert(v != NULL);                                                         \
    assert(v->data != NULL);                                                   \
    assert(v->size <= v->capacity);                                            \
  }                                                                            \
                                                                               \
  void __name##_v_expand(name##_vector_t *v) {                                 \
    __name##_v_sanity_check(v);                                                \
                                                                               \
    if (v->capacity < 2) {                                                     \
      v->capacity = 2;                                                         \
    } else {                                                                   \
      bool overflowed =                                                        \
          __builtin_uadd_overflow(v->capacity, v->capacity / 2, &v->capacity); \
      assert(!overflowed);                                                     \
    }                                                                          \
                                                                               \
    v->data = realloc(v->data, v->capacity * sizeof(T));                       \
    __name##_v_sanity_check(v);                                                \
  }                                                                            \
                                                                               \
  name##_vector_t *name##_vector_create() {                                    \
    name##_vector_t *vector = malloc(sizeof(name##_vector_t));                 \
    assert(vector != NULL);                                                    \
                                                                               \
    vector->capacity = 0;                                                      \
    vector->size = 0;                                                          \
    vector->data = malloc(0);                                                  \
    assert(vector->data != NULL);                                              \
                                                                               \
    return vector;                                                             \
  }                                                                            \
                                                                               \
  name##_vector_t *name##_vector_create_with_capacity(unsigned capacity) {     \
    name##_vector_t *vector = malloc(sizeof(name##_vector_t));                 \
    assert(vector != NULL);                                                    \
                                                                               \
    vector->capacity = capacity;                                               \
    vector->size = 0;                                                          \
    vector->data = malloc(capacity * sizeof(T));                               \
    assert(vector->data != NULL);                                              \
                                                                               \
    return vector;                                                             \
  }                                                                            \
                                                                               \
  unsigned name##_vector_size(name##_vector_t *v) {                            \
    __name##_v_sanity_check(v);                                                \
    return v->size;                                                            \
  }                                                                            \
                                                                               \
  T name##_vector_at(name##_vector_t *v, unsigned index) {                     \
    __name##_v_sanity_check(v);                                                \
    assert(index < v->size);                                                   \
    return v->data[index];                                                     \
  }                                                                            \
                                                                               \
  void name##_vector_append(name##_vector_t *v, T item) {                      \
    __name##_v_sanity_check(v);                                                \
                                                                               \
    if (v->size == v->capacity) {                                              \
      __name##_v_expand(v);                                                    \
    }                                                                          \
                                                                               \
    v->data[v->size++] = item;                                                 \
  }                                                                            \
                                                                               \
  void name##_vector_swap(name##_vector_t *v, unsigned i1, unsigned i2) {      \
    __name##_v_sanity_check(v);                                                \
    assert(i1 < v->size);                                                      \
    assert(i2 < v->size);                                                      \
                                                                               \
    T tmp = v->data[i1];                                                       \
    v->data[i1] = v->data[i2];                                                 \
    v->data[i2] = tmp;                                                         \
  }                                                                            \
                                                                               \
  T name##_vector_remove(name##_vector_t *v, unsigned index) {                 \
    __name##_v_sanity_check(v);                                                \
    assert(index < v->size);                                                   \
                                                                               \
    T removed = v->data[index];                                                \
    v->data[index] = v->data[--v->size];                                       \
    return removed;                                                            \
  }                                                                            \
                                                                               \
  void name##_vector_destroy(name##_vector_t *v) {                             \
    __name##_v_sanity_check(v);                                                \
                                                                               \
    free(v->data);                                                             \
    free(v);                                                                   \
  }

#endif