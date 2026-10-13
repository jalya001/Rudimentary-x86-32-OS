#pragma once

#include <minstd/types.hpp>
#include <minstd/utils.hpp>
#include <minstd/string.hpp>
#include "../sync.hpp"
#include "../print.hpp"

#define FREE 0
#define USED 1

// note these are inside their own data area

// later we need an option of "many-to-few" for hashmap pools
// not the same as the conceptual definition of pool. this is a FIXED SIZE POOL managed by bitmap
template<typename DATA_TYPE = uint8_t, typename INT_TYPE = uint32_t>
struct Pool {
  INT_TYPE max_entries;
  //uintptr_t cursor; // caches the spatial next because it is likelier to be unused. 
  uint8_t *bitmap;
  DATA_TYPE *datamap;
  Lock bitmap_lock;

  INT_TYPE allocate() {
    this->bitmap_lock.acquire();
    for (INT_TYPE i = 0; i < this->max_entries; i ++) {
      if (bitmap_get<INT_TYPE>(this->bitmap, i) == FREE) {
        bitmap_mark<INT_TYPE>(this->bitmap, i);
        this->bitmap_lock.release();
        return i;
      }
    }
    this->bitmap_lock.release();
    return -1; //ERR_POOL_FULL;
  }
  void free(INT_TYPE id) {
    this->bitmap_lock.acquire();
    bitmap_unmark<INT_TYPE>(this->bitmap, id);
    this->bitmap_lock.release();
  }
  
  DATA_TYPE *id_to_address(INT_TYPE id) {
    return &(this->datamap[id]);
  }
  INT_TYPE address_to_id(DATA_TYPE *address) { // needs to do a bounds and divisibility check?
    return ((uintptr_t)address - (uintptr_t)this->datamap) / sizeof(DATA_TYPE);
  }
};

/* This assumes you're giving it valid and proportional memory */
template<typename DATA_TYPE = uint8_t, typename INT_TYPE = uint32_t>
Pool<DATA_TYPE> *_create_pool(uintptr_t start, size_t size) { // pool_limit is how many DATA the pool fits
  uint8_t *ptr = (uint8_t *)start;
  INT_TYPE max_entries = (INT_TYPE)((8 * size) / (8 * sizeof(DATA_TYPE) + 1));

  Pool<DATA_TYPE> *pool = (Pool<DATA_TYPE> *)ptr;
  ptr += sizeof(Pool<DATA_TYPE>);
  uint8_t *bitmap = ptr;
  INT_TYPE bitmap_bytes = divide_roundup<INT_TYPE>(max_entries, 8); // is there nothing we can do to simplify?
  buffer_zero((char *)ptr, bitmap_bytes); // not part of the below loop under the assumption there is an optimized mass zeroing thing
  ptr += bitmap_bytes;
  DATA_TYPE *datamap = (DATA_TYPE *)ptr;
  for (INT_TYPE i = 0; i < max_entries; i++) datamap[i] = {};
  ptr += max_entries * sizeof(DATA_TYPE);

  *pool = { max_entries, /*0,*/ bitmap, datamap };
  return pool;
}
