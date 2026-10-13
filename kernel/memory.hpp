#pragma once

#include <minstd/types.hpp>
#include "structs/clusterpool.hpp"

#define MEM_CLUSTER_POOLS_MAX 64
#define MEM_CLUSTERS_MAX 2048
#define ALL_MEM_CLUSTER_POOLS_START 0x120000
constexpr uintptr_t ALL_MEM_CLUSTER_POOLS_BYTES = MEM_CLUSTER_POOLS_MAX * sizeof(ClusterPool);
constexpr uintptr_t ALL_MEM_CLUSTERS_START = ALL_MEM_CLUSTER_POOLS_START + ALL_MEM_CLUSTER_POOLS_BYTES;
constexpr uintptr_t ALL_MEM_CLUSTERS_BYTES = MEM_CLUSTERS_MAX * sizeof(Cluster);
constexpr uintptr_t KPOOL_START = ALL_MEM_CLUSTERS_START + ALL_MEM_CLUSTERS_BYTES;
#define KPOOL_BYTES (256 * 1024) // 256KB for kernel pool

void init_memory();
uintptr_t kmalloc(size_t size);
uintptr_t kcalloc(size_t size);
void kfree(uintptr_t address); // switch to void pointer?

ClusterPool *_create_cluster_pool(uintptr_t where, uintptr_t start, size_t size, Pool<Cluster> *node_pool);