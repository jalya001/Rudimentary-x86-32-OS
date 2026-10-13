#pragma once

#include <minstd/types.hpp>
#include "entry.hpp"
#include "structs/stack.hpp"
#include "structs/pool.hpp"
#include "structs/controlblock.hpp"
#include "print.hpp"

#define STACKS_START 0x20000
#define STACK_SIZE 0x2000
#define STACKS_END 0x60000
#define MAX_STACKS 32

#define PROCESS_LIMIT 8
#define THREAD_LIMIT 12

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

struct IdAllocator {
  uint32_t next;
  uint32_t limit;
  
  uint32_t allocate_id() { // Temporary approach. This is bound to run out of PIDs eventually
    next++;
    if (next < 1) return -1; // if it wrapped around
    if (next > limit) return -1;
    return next;
  }
};

typedef int (*syscall_t) (...);
extern syscall_t syscalls[256];
extern tcb_t *current_running;
extern tcb_t *zombie;   // thread that exited last cycle: its stacks are reaped on the next switch
extern IdAllocator tid_allocator;
extern StackAllocator stack_allocator;
extern Thread thread_pool[THREAD_LIMIT];

struct SwitchFrame {
  uint32_t gs, fs, es, ds;
  uint32_t edi, esi, ebp, esp_unused, ebx, edx, ecx, eax;
  uint32_t return_eip;
};

void halt();
