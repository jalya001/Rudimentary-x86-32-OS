#pragma once

#include <minstd/types.hpp>

struct Stack {
  uintptr_t bottom;
  uintptr_t sp;

  void configure_stack(uintptr_t in_bottom, uintptr_t size) {
    this->bottom = in_bottom;
    this->sp = this->bottom + size;
  }
};
