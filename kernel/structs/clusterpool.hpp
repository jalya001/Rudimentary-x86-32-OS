#pragma once

#include <minstd/types.hpp>
#include <minstd/utils.hpp>
#include "../sync.hpp"
#include "pool.hpp"

#define FREE 0
#define USED 1

/* TODO:
 * - Make "find one fit that seems neatest or at least very neat" rather than "first fit" for cluster pool allocations
 */

struct ClusterPool;

struct Cluster {
  ClusterPool *parent;
  uintptr_t start;
  size_t size;
  Cluster *next;
  Cluster *prev; // needed for coalescing
  bool type; // 0 for free, 1 for used

  uintptr_t splituse(size_t size);
  void free();
  void coalesce_next();
  uintptr_t limit() { return this->start + this->size; }
};

struct ClusterPool {
  Cluster *cursor; // because we use the policy of finding the first available cluster from the last one
  uintptr_t start;
  size_t size;
  Pool<Cluster> *nodes;
  // every ClusterPool should use the same pool for storing metadata
  // arbitrary indexing is not used, only linked list. because no hashmap. free is therefore O(n) average
  size_t bytes_used; // is not allowed to exceed size
  Lock lock;

  uintptr_t allocate(size_t size);
  void free(uintptr_t address);
  uintptr_t limit() { return this->start + this->size; }
};

