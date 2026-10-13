#include <minstd/string.hpp>
#include <minstd/types.hpp>
#include <minstd/utils.hpp>
#include "clusterpool.hpp"
#include "../sync.hpp"
#include "pool.hpp"
#include "../print.hpp"

/*
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
  */

uintptr_t ClusterPool::allocate(size_t req_size) {
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
  //kprintf("rt = %x\n", rt);
  this->cursor = test;
  //debug_print_nodes();
  this->lock.release();
  return rt;
}
void ClusterPool::free(uintptr_t address) { // does an excruciating O(n) search by moving the cursor backwards
  // need a case for attempting to free if only one node remains. that should fail
  this->lock.acquire();
  Cluster *candidate = this->cursor;

  do {
    if (candidate->start == address) {
      this->cursor = candidate->prev;
      candidate->free();
      this->lock.release();
      return;
    }
    candidate = candidate->prev;
  } while (candidate != this->cursor);

  this->lock.release();
  kprintf("ERROR: FREE NOT FOUND %x\n", address);
}

uintptr_t Cluster::splituse(size_t new_size) {
  Cluster *new_cluster = this->parent->nodes->id_to_address(this->parent->nodes->allocate());
  //kprintf("new cluster: %x\n", new_cluster);
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
  this->parent->nodes->free(this->parent->nodes->address_to_id(hold));
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