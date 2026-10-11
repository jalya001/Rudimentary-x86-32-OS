#pragma once

#include <minstd/types.hpp>

/* MinSTD provides most functions which would otherwise be found in the standard library. */
/* This here is for all the functions that do not get a dedicated file */

void srand(uint32_t seed);
int rand(void);
 
int atoi(const char *s);
void dtoa(double dbl, char *s, int s_len);
void itoa(uint32_t n, char *s);
void itohex(uint32_t n, char *s);

uint32_t ntohl(uint32_t data);
uint32_t htonl(uint32_t data);
uint16_t ntohs(uint16_t data);
uint16_t htons(uint16_t data);

template <typename INTEGER_TYPE>
bool bitmap_get(const uint8_t* bitmap, INTEGER_TYPE index) {
  auto byte = index / 8;
  auto bit = index % 8;

  return (bitmap[byte] & (1u << bit)) != 0;
}

template <typename INTEGER_TYPE>
void bitmap_mark(uint8_t* bitmap, INTEGER_TYPE index) {
  auto byte = index / 8;
  auto bit = index % 8;

  bitmap[byte] |= (uint8_t)(1u << bit);
}

template <typename INTEGER_TYPE>
void bitmap_unmark(uint8_t* bitmap, INTEGER_TYPE index) {
  auto byte = index / 8;
  auto bit = index % 8;

  bitmap[byte] &= (uint8_t)~(1u << bit);
}
template <typename INTEGER_TYPE>
INTEGER_TYPE divide_roundup(INTEGER_TYPE a, INTEGER_TYPE b) {
  return (a + b - 1) / b;
}