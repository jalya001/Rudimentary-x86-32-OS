#pragma once

#include <minstd/types.hpp>
#include "structs/pool.hpp"
#include "sync.hpp"

struct ID {
  uintptr_t ptr;
  uint16_t count;
  Lock lock;
};

struct IDAllocator {
  Pool<ID> *pool;
  void (*reaping_func)(uintptr_t);

  uint32_t next(uintptr_t ptr);
  void open(uint32_t id);
  void close(uint32_t id);
};

void init_ids();
uint32_t create_id_allocator();

extern IDAllocator *object_allocator;
extern IDAllocator *pid_allocator;
extern IDAllocator *tid_allocator;
