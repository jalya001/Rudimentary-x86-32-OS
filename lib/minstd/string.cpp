#include "string.hpp"

void buffer_copy(const char *source, char *destin, int size) {
  for (int i = 0; i < size; ++i) destin[i] = source[i];
}

void buffer_zero(char *a, int size) {
  for (int i = 0; i < size; ++i) a[i] = 0;
}

int strcmp(const char *s, const char *t) {
  while (*s && *s == *t) { s++; t++; }
  return (unsigned char)*s - (unsigned char)*t;
}