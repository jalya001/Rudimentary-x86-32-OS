#pragma once

#include <minstd/types.hpp>

#define STACKS_START 0x20000
#define STACK_SIZE 0x2000
#define STACKS_END 0x40000

struct Process;
struct Thread;
struct AddressSpace;

enum TaskState {
  READY,
  RUNNING,
  BLOCKED,
  EXITED
};

struct Stack {
  uintptr_t base;
  uintptr_t sp;
};

typedef struct Thread {
  uint32_t tid;
  TaskState state;
  Stack kernel_stack;
  Stack user_stack;
  Thread *next;
  Thread *prev;
} tcb_t;

struct AddressSpace {
  uintptr_t begin;
  uintptr_t end;
};

typedef struct Process {
  uint32_t pid;
  TaskState state;
  AddressSpace* address_space;
  uint32_t tid; // temporarily used to index table
} pcb_t;

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




void fd_write(int fd, const char *msg);