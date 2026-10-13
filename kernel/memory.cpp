#include <minstd/string.hpp>
#include <minstd/types.hpp>
#include <minstd/utils.hpp>
#include "sync.hpp"
#include "memory.hpp"
#include "print.hpp"

Pool<ClusterPool> *all_mem_cluster_pools;
Pool<Cluster> *all_mem_clusters;

ClusterPool *kpool;

static void debug_print_nodes() {
  Cluster *test = kpool->cursor->next;
  kprintf("CURSOR: %x-%x\n", kpool->cursor->start, kpool->cursor->limit());
  int i = 0;
  while (test != kpool->cursor) {
    kprintf("NODE %d: %x-%x\n", i, test->start, test->limit());
    test = test->next;
    i++;
  }
}

/* This assumes you're giving it valid and proportional memory */
template<typename DATA_TYPE = uint8_t>
static Pool<DATA_TYPE> *_create_pool(uintptr_t max_entries, uintptr_t start, uintptr_t size) { // pool_limit is how many DATA the pool fits
  uint8_t *ptr = (uint8_t *)start;

  Pool<DATA_TYPE> *pool = (Pool<DATA_TYPE> *)ptr;
  ptr += sizeof(Pool<DATA_TYPE>);
  uint8_t *bitmap = ptr;
  uintptr_t bitmap_bytes = divide_roundup<uintptr_t>(max_entries, 8);
  buffer_zero((char *)ptr, bitmap_bytes); // not part of the below loop under the assumption there is an optimized mass zeroing thing
  ptr += bitmap_bytes;
  DATA_TYPE *datamap = (DATA_TYPE *)ptr;
  for (uintptr_t i = 0; i < max_entries; i++) datamap[i] = {};
  ptr += max_entries * sizeof(DATA_TYPE);

  *pool = { max_entries, 0, bitmap, datamap };
  return pool;
}

static ClusterPool *_create_cluster_pool(uintptr_t where, uintptr_t start, uintptr_t size, Pool<Cluster> *node_pool) {
  ClusterPool *pool = (ClusterPool *)where;
  Cluster *first = node_pool->id_to_paddress(node_pool->allocate());

  *pool = { first, start, size, node_pool, 0 };
  *first = { pool, start, size, first, first, FREE};
  return pool;
}

static ClusterPool *_create_memory_cluster_pool(uintptr_t start, uintptr_t size) {
  ClusterPool *pool = all_mem_cluster_pools->id_to_paddress(all_mem_cluster_pools->allocate());
  return _create_cluster_pool((uintptr_t)pool, start, size, all_mem_clusters);
}

uintptr_t ClusterPool::allocate(uintptr_t req_size) {
  this->lock.acquire();
  //debug_print_nodes();
  if (this->size < this->bytes_used + req_size) {
    kprintf("ERROR: CLUSTER POOL NOT ENOUGH TOTAL SPACE\n\0");
    this->lock.release();
    return 0;
  }
  Cluster *test = this->cursor; // or do a for loop of the entire pool size instead?
  while (test->type == USED || test->size < req_size) {
    test = test->next;
    if (test == this->cursor) {
      kprintf("ERROR: CLUSTER POOL NO FIT\n\0");
      this->lock.release();
      return 0; //return ERR_POOL_FULL; // NEEDS TO KILL AND NOT GIVE 0 BECAUSE 0 IS A VALID INDEX
    }
  }
  this->bytes_used += req_size;
  uintptr_t rt;
  if (test->size == req_size) {
    test->type = USED;
    rt = test->start;
  } else {
    rt = test->splituse(req_size);
  }
  this->cursor = test;
  //debug_print_nodes();
  this->lock.release();
  return rt;
}

void ClusterPool::free(uintptr_t address) { // does an excruciating O(n) search by moving the cursor backwards
  this->lock.acquire();
  Cluster *start = this->cursor;
  Cluster *test = this->cursor;
  
  do {
    if (test->start == address) {
      this->cursor = test->prev;
      test->free();
      this->lock.release();
      return;
    }
    test = test->prev;
  } while (test != start);

  this->lock.release();
  kprintf("ERROR: FREE NOT FOUND %x\n", address);
}

uintptr_t Cluster::splituse(uintptr_t new_size) {
  Cluster *new_cluster = this->parent->nodes->id_to_paddress(this->parent->nodes->allocate());
  //kprintf("REQ: %d\n", new_size);
  this->size -= new_size;
  //kprintf("THIS->LIMIT: %x\n", this->limit());
  *new_cluster = { this->parent, this->limit(), new_size, this->next, this, USED };
  this->next->prev = new_cluster;
  this->next = new_cluster;
  //kprintf("NEW CLUSTER START: %x\n", new_cluster->start);
  //kprintf("THIS->LIMIT: %x\n", this->limit());
  //kprintf("THIS->PARENT->CURSORSR: %x\n", this->parent->cursor->limit());
  return new_cluster->start;
}

void Cluster::coalesce_next() {
  if (this->next == this) return; // not allowed to merge itself
  Cluster *hold = this->next;
  this->size += this->next->size;
  this->next = this->next->next;
  this->next->prev = this;
  this->parent->nodes->free(this->parent->nodes->paddress_to_id(hold));
}

// note the main way to free user processes is not to
// rely on them explicitly calling this, but the fact their pool dies when they die
void Cluster::free() {
  if (this->type == FREE) return;
  this->type = FREE;
  if (
    this->next->type == FREE
    && (this->limit() != this->parent->limit()) // not allowed to merge past boundary
  ) {
    //kprintf("COALESCE FROM THIS (%x-%x) TO NEXT (%x-%x)\n", this->start, this->limit(), this->next->start, this->next->limit());
    this->coalesce_next();
  }
  if (
    this->prev->type == FREE
    && (this->next != this->prev) // not allowed to merge the same node twice
  ) {
    //kprintf("COALESCE FROM PREV (%x-%x) TO THIS (%x-%x)\n", this->start, this->limit(), this->next->start, this->next->limit());
    this->prev->coalesce_next();
  }
}

uintptr_t kmalloc(uintptr_t size) {
  return kpool->allocate(size);
}

uintptr_t kcalloc(uintptr_t size) {
  uintptr_t allocation = kmalloc(size);
  buffer_zero((char *)allocation, size);
  return allocation;
}

void kfree(uintptr_t address) {
  kpool->free(address);
}

void init_memory() {
  all_mem_cluster_pools = _create_pool<ClusterPool>(MEM_CLUSTER_POOLS_MAX, ALL_MEM_CLUSTER_POOLS_START, ALL_MEM_CLUSTER_POOLS_BYTES);
  all_mem_clusters = _create_pool<Cluster>(MEM_CLUSTERS_MAX, ALL_MEM_CLUSTERS_START, ALL_MEM_CLUSTERS_BYTES);

  kpool = _create_memory_cluster_pool(KPOOL_START, KPOOL_BYTES);
  kprintf("KPOOL STARTED AT: %x-%x\n", kpool->start, kpool->limit());
}
