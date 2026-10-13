#pragma once

#include <minstd/types.hpp>
#include "memory.hpp"

template<typename DATA_TYPE = uint8_t, typename INT_TYPE uint32_t>
Pool<DATA_TYPE> *kcreate_pool(INT_TYPE max_entries) { // assumes memory has been inited
  size_t size = max_entries * sizeof(DATA_TYPE) + sizeof(Pool<DATA_TYPE>);
  uintptr_t start = kmalloc(size);
  return _create_pool(start, size);
}
