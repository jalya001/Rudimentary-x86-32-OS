#include "string.hpp"

void buffer_copy(const char *source, char *destin, int size) {
  for (int i = 0; i < size; ++i) destin[i] = source[i];
}

void buffer_zero(char *a, int size) {
  for (int i = 0; i < size; ++i) a[i] = 0;
}