#pragma once

#include <minstd/types.hpp>
#include <minstd/utils.hpp>
#include "sync.hpp"

#define FREE 0
#define USED 1

/* TODO:
 * - Make "find one fit that seems neatest or at least very neat" rather than "first fit" for cluster pool allocations
 */

// later we need an option of "many-to-few" for hashmap pools
// not the same as the conceptual definition of pool. this is a FIXED SIZE POOL managed by bitmap
template<typename DATA_TYPE = uint8_t>
struct Pool {
  uintptr_t limit;
  uintptr_t cursor; // caches the spatial next because it is likelier to be unused. 
  uint8_t *bitmap;
  DATA_TYPE *datamap;
  Lock bitmap_lock;

  uintptr_t allocate() {
    this->bitmap_lock.acquire();
    for (uintptr_t i = 0; i < this->limit; i ++) {
      if (bitmap_get<uintptr_t>(this->bitmap, i) == FREE) {
        bitmap_mark<uintptr_t>(this->bitmap, i);
        this->bitmap_lock.release();
        return i;
      }
    }
    this->bitmap_lock.release();
    return 0; //ERR_POOL_FULL;
  }
  void free(uintptr_t id) {
    this->bitmap_lock.acquire();
    bitmap_unmark<uintptr_t>(this->bitmap, id);
    this->bitmap_lock.release();
  }
  
  DATA_TYPE *id_to_paddress(uintptr_t id) {
    return &(this->datamap[id]);
  }
  uintptr_t paddress_to_id(DATA_TYPE *address) {
    return ((uintptr_t)address - (uintptr_t)this->datamap) / sizeof(DATA_TYPE);
  }
};

struct ClusterPool;

struct Cluster {
  ClusterPool *parent;
  uintptr_t start;
  uintptr_t size;
  Cluster *next;
  Cluster *prev; // needed for coalescing
  uint8_t type; // 0 for free, 1 for used

  uintptr_t splituse(uintptr_t size);
  void free();
  void coalesce_next();
  uintptr_t limit() { return this->start + this->size; }
};

struct ClusterPool {
  Cluster *cursor; // because we use the policy of finding the first available cluster from the last one
  uintptr_t start;
  uintptr_t size;
  Pool<Cluster> *nodes;
  // every ClusterPool should use the same pool for storing metadata
  // arbitrary indexing is not used, only linked list. because no hashmap. free is therefore O(n) average
  uintptr_t bytes_used; // is not allowed to exceed size
  Lock lock;

  uintptr_t allocate(uintptr_t size);
  void free(uintptr_t address);
  uintptr_t limit() { return this->start + this->size; }
};

#define MEM_CLUSTER_POOLS_MAX 64
#define MEM_CLUSTERS_MAX 2048
#define ALL_MEM_CLUSTER_POOLS_START 0x60000
constexpr uintptr_t ALL_MEM_CLUSTER_POOLS_BYTES = MEM_CLUSTER_POOLS_MAX * sizeof(ClusterPool);
constexpr uintptr_t ALL_MEM_CLUSTERS_START = ALL_MEM_CLUSTER_POOLS_START + ALL_MEM_CLUSTER_POOLS_BYTES;
constexpr uintptr_t ALL_MEM_CLUSTERS_BYTES = MEM_CLUSTERS_MAX * sizeof(Cluster);
constexpr uintptr_t KPOOL_START = ALL_MEM_CLUSTERS_START + ALL_MEM_CLUSTERS_BYTES;
#define KPOOL_BYTES (256 * 1024) // 256KB for kernel pool

void init_memory();
uintptr_t kmalloc(uintptr_t size);
uintptr_t kcalloc(uintptr_t size);
void kfree(uintptr_t address);
