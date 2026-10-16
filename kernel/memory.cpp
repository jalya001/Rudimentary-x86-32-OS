#include <minstd/string.hpp>
#include <minstd/types.hpp>
#include "memory.hpp"
#include "print.hpp"
#include "structs/clusterpool.hpp"
#include "structs/pool.hpp"
#include "kernel.hpp"

Pool<ClusterPool> *all_mem_cluster_pools;
Pool<Cluster> *all_mem_clusters;

ClusterPool *kpool;

ClusterPool *_create_cluster_pool(uintptr_t where, uintptr_t start, size_t size, Pool<Cluster> *node_pool) {
  ClusterPool *pool = (ClusterPool *)where;
  Cluster *first = node_pool->id_to_address(node_pool->allocate());

  *pool = { first, start, size, node_pool, 0 };
  *first = { pool, start, size, first, first, FREE};
  return pool;
}

static ClusterPool *_create_memory_cluster_pool(uintptr_t start, size_t size) {
  ClusterPool *pool = all_mem_cluster_pools->id_to_address(all_mem_cluster_pools->allocate());
  return _create_cluster_pool((uintptr_t)pool, start, size, all_mem_clusters);
}

uintptr_t kmalloc(size_t size) {
  return kpool->allocate(size);
}

uintptr_t kcalloc(size_t size) {
  uintptr_t allocation = kmalloc(size);
  buffer_zero((char *)allocation, size);
  return allocation;
}

void kfree(uintptr_t address) {
  kpool->free(address);
}

void init_memory() {
  all_mem_cluster_pools = _create_pool<ClusterPool, uint32_t>(MEM_CLUSTER_POOLS_MAX, ALL_MEM_CLUSTER_POOLS_START, ALL_MEM_CLUSTER_POOLS_BYTES);
  all_mem_clusters = _create_pool<Cluster, uint32_t>(MEM_CLUSTERS_MAX, ALL_MEM_CLUSTERS_START, ALL_MEM_CLUSTERS_BYTES);

  kpool = _create_memory_cluster_pool(KPOOL_START, KPOOL_BYTES);
  kprintf("KPOOL STARTED AT: %x-%x\n", kpool->start, kpool->limit());
}
