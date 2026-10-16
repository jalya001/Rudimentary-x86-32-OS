#include "id.hpp"
#include <minstd/types.hpp>
#include <minstd/utils.hpp>
#include <minstd/string.hpp>
#include "sync.hpp"
#include "structs/pool.hpp"
#include "kstructs.hpp"
#include "memory.hpp"
#include "kernel.hpp"

IDAllocator *object_allocator;
IDAllocator *pid_allocator;
IDAllocator *tid_allocator;

// idallocator composes an id pool
static IDAllocator* _create_id_allocator(uint32_t max_entries, void (*reaping_func)(uintptr_t)) {
  Pool<ID> *id_pool = (Pool<ID> *)kcreate_pool<ID>(max_entries);
  IDAllocator *rt = (IDAllocator *)kmalloc(sizeof(IDAllocator));
  *rt = { id_pool, reaping_func };
  return rt;
}

uint32_t IDAllocator::next(uintptr_t ptr) {
  uint32_t id = this->pool->allocate();
  ID *id_mem = this->pool->id_to_address(id);
  *id_mem = { ptr, 1 };
  return id;
}

/* Note that since there is no tracking of what has opened it,
 * this is not safe for user processes to arbitrarily invoke
 * and need to go through sorts of wrappers fully consistent in opening and closing */
void IDAllocator::open(uintptr_t id) {
  this->pool->datamap[id].lock.acquire();
  if (this->pool->datamap[id].count == 0) return;
  this->pool->datamap[id].count++;
  // need a bounds check for count
  this->pool->datamap[id].lock.release();
}

void IDAllocator::close(uintptr_t id) {
  this->pool->datamap[id].lock.acquire();
  if (this->pool->datamap[id].count == 0) return;
  this->pool->datamap[id].count--;
  if (this->pool->datamap[id].count == 0) {
    this->reaping_func(this->pool->datamap[id].ptr);
    this->pool->free(id);
  }
  this->pool->datamap[id].lock.release();
}

void dummy_idk(uintptr_t asdqw) {
  kprintf("I do nothing\n");
  return;
}

void init_ids() {
  object_allocator = _create_id_allocator(128, dummy_idk);
  pid_allocator = _create_id_allocator(128, dummy_idk);
  tid_allocator = _create_id_allocator(128, thread_reap_delayed);
}