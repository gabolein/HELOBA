#ifndef VECTOR_H
#define VECTOR_H

#include <stdbool.h>

typedef struct {
  unsigned capacity;
  unsigned size;
  int *data;
} vector_t;

vector_t *vector_create();
vector_t *vector_create_with_capacity(unsigned capacity);
unsigned vector_size(vector_t *v);
bool vector_full(vector_t *v);
int vector_at(vector_t *v, unsigned index);
void vector_append(vector_t *v, int item);
void vector_swap(vector_t *v, unsigned i1, unsigned i2);
int vector_remove(vector_t *v, unsigned index);
void vector_destroy(vector_t *v);

#endif