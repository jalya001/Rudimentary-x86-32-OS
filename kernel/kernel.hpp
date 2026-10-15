#pragma once

#include <minstd/types.hpp>
#include "entry.hpp"
#include "structs/stack.hpp"
#include "structs/pool.hpp"
#include "structs/controlblock.hpp"
#include "print.hpp"

// temporary stuff to be removed
#define STACKS_START 0x20000
#define STACK_SIZE 0x2000
#define STACKS_END 0x60000
#define MAX_STACKS 32

//#define PROCESS_LIMIT 8
#define THREAD_LIMIT 12

/*
struct StackAllocator {
  Pool<Stack> *pool;

  Stack *allocate() {
    uint32_t id = this->pool->allocate();
    if (id == (uint32_t)-1) return nullptr;
    uintptr_t start = STACKS_START + id * STACK_SIZE;
    this->pool->datamap[id].configure_stack(start, STACK_SIZE);
    kprintf("NEW STACK AT: bottom: %x, sp: %x\n", this->pool->datamap[id].bottom, this->pool->datamap[id].sp);
    return this->pool->id_to_address(id);
  }

  void free(Stack *stack) {
    uint32_t id = this->pool->address_to_id(stack);
    this->pool->free(id);
  }
};
*/
typedef int (*syscall_t) (...);
extern syscall_t syscalls[256];
extern tcb_t *current_running;
//extern StackAllocator stack_allocator;
extern Thread thread_pool[THREAD_LIMIT];

struct SwitchFrame {
  uint32_t gs, fs, es, ds;
  uint32_t edi, esi, ebp, esp_unused, ebx, edx, ecx, eax;
  uint32_t return_eip;
};

void thread_reap(Thread *t);
void thread_reap_delayed(uintptr_t where);
void halt();
