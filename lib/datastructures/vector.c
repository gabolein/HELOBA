#include "lib/datastructures/vector.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool vector_is_valid(vector_t *v);
bool vector_expand(vector_t *v);

bool vector_is_valid(vector_t *v) {
  if (v == NULL || v->data == NULL || v->size > v->capacity) {
    fprintf(stderr,
            "[WARNING] Vector @ %p was found to be in an inconsistent state.\n",
            v);
    return false;
  }

  return true;
}

bool vector_expand(vector_t *v) {
  if (!vector_is_valid(v))
    return false;

  if (v->capacity < 2)
    v->capacity = 2;
  else if (__builtin_uadd_overflow(v->capacity, v->capacity / 2, &v->capacity))
    return false;

  v->data = realloc(v->data, v->capacity * sizeof(int));
  return vector_is_valid(v);
}

vector_t *vector_create() {
  vector_t *vector = malloc(sizeof(vector_t));
  if (vector == NULL) {
    fprintf(stderr, "[WARNING] malloc returned NULL.\n");
    return NULL;
  }

  vector->capacity = 0;
  vector->size = 0;
  vector->data = malloc(0);
  if (vector->data == NULL) {
    fprintf(stderr, "[WARNING] malloc returned NULL.\n");
    return NULL;
  }

  return vector;
}

vector_t *vector_create_with_capacity(unsigned capacity) {
  vector_t *vector = malloc(sizeof(vector_t));
  if (vector == NULL) {
    fprintf(stderr, "[WARNING] malloc returned NULL.\n");
    return NULL;
  }

  vector->capacity = capacity;
  vector->size = 0;
  vector->data = malloc(capacity * sizeof(int));
  if (vector->data == NULL) {
    fprintf(stderr, "[WARNING] malloc returned NULL.\n");
    return NULL;
  }

  return vector;
}

unsigned vector_size(vector_t *v) { return v->size; }

bool vector_at(vector_t *v, unsigned index, int *out) {
  if (!vector_is_valid(v))
    return false;

  if (index >= v->size) {
    fprintf(
        stderr,
        "[WARNING] Index %u is out of bounds for Vector @ %p with size = %u\n",
        index, v, v->size);
    return false;
  }

  *out = v->data[index];
  return true;
}

bool vector_insert(vector_t *v, int item) {
  if (v->size == v->capacity && vector_expand(v) == false) {
    fprintf(stderr, "[WARNING] Couldn't expand Vector @ %p.\n", v);
    return false;
  }

  v->data[v->size++] = item;
  return true;
}

bool vector_remove(vector_t *v, unsigned index) {
  if (!vector_is_valid(v))
    return false;

  if (index >= v->size) {
    fprintf(
        stderr,
        "[WARNING] Index %u is out of bounds for Vector @ %p with size = %u\n",
        index, v, v->size);
    return false;
  }

  v->data[index] = v->data[--v->size];
  return true;
}

bool vector_destroy(vector_t *v) {
  if (!vector_is_valid(v))
    return false;

  free(v->data);
  free(v);
  return true;
}