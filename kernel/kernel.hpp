#pragma once

#include <minstd/types.hpp>
#include "entry.hpp"
#include "structs/stack.hpp"
#include "structs/pool.hpp"
#include "structs/controlblock.hpp"
#include "print.hpp"

// Kernel stacks live at 1MB–1MB+96KB, safely above the kernel BSS
// (which can reach ~0x56000 due to large page-table arrays in paging.cpp).
// DO NOT use addresses below 0x100000 here.
#define STACKS_START 0x100000
#define STACK_SIZE   0x2000
#define STACKS_END   0x118000

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

// Non-template wrapper around thread_create<true> for use in other TUs.
tcb_t *thread_create_user(void (*entry_fn)());