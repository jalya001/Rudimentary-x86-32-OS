#pragma once

#include <minstd/types.hpp>

struct IDTE {
  uint16_t offset_low;
  uint16_t selector;
  uint8_t reserved;
  uint8_t flags;
  uint16_t offset_high;
} __attribute__((packed));

struct IDTPTR {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));

void init_interrupts();