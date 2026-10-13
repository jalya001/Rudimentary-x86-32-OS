#pragma once

#include "stack.hpp"

enum TaskState {
  READY,
  RUNNING,
  BLOCKED,
  EXITED
};

typedef struct Thread {
  uint32_t tid;
  TaskState state;
  Stack *kernel_stack; // to be spread across less pages, these shouldn't be pointers, actually?
  // for that, we should have "headless pools" which only have content and bitmap, but not a stow of nodes
  Stack *user_stack;
  uint32_t *page_directory;
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
  //Permission permissions[16]; // for now they simply have a dumb contiguous list, since we don't have memory allocation
} pcb_t;