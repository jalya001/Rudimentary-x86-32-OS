#include "drivers/serial.hpp"
#include "drivers/vga.hpp"
#include "kernel.hpp"
#include "gdt.hpp"
#include "entry.hpp"
#include "scheduler.hpp"
#include "interrupts.hpp"
#include <common.hpp>
#include "test.hpp"
#include "pic.hpp"

extern "C" void kernel_main();

tcb_t *current_running = 0;
syscall_t syscalls[256];

#define PROCESS_LIMIT 8
#define THREAD_LIMIT 12

IdAllocator pid_allocator = { 1, PROCESS_LIMIT }; // temporarily, we are using id for indexing tables 
IdAllocator tid_allocator = { 1, THREAD_LIMIT };
Process* processes[PROCESS_LIMIT];
Thread thread_pool[THREAD_LIMIT];     // static pool: no heap/allocator exists yet

void fd_write(int fd, const char *msg) { // fd not used yet though
  serial_print(msg);
  vga_write(msg);
}

extern "C" void fault_print(const char* name, uint32_t error) {
  kprintf("%s=%d\n", name, error);
}

void init_syscalls() {
  syscalls[SYS_YIELD] = (syscall_t)yield;
  syscalls[SYS_EXIT] = (syscall_t)exit;
  syscalls[SYS_WRITE] = (syscall_t)fd_write;
}

extern "C" void syscall_handler(uint32_t syscall_id, uint32_t arg1, uint32_t arg2, uint32_t arg3) { // could have used the trapframe instead but lazy
  syscalls[syscall_id](arg1, arg2, arg3);
}

/*
 * Creates a new thread and splices it into the circulat ready queue.
 * `user` picks whether it starts in ring 0 or ring 3 on first dispatch.
 * NOTE: only supports up to 8 threads right now, see stack layout comment
 * in scheduler.cpp. 
 */
tcb_t *thread_create(void (*entry_fn)(), bool user) {
  uint32_t tid = tid_allocator.allocate_id();
  if (tid == (uint32_t) - 1) return 0;

  tcb_t *t = &thread_pool[tid - 1];
  *t = {};
  t->tid = tid;
  t->state = READY;
  t->kernel_stack.base = STACKS_START + (tid * 2) * STACK_SIZE;
  t->kernel_stack.sp = t->kernel_stack.base;

  if (user) {
    uintptr_t user_stack = STACKS_START + (tid * 2 + 1) * STACK_SIZE;
    t->user_stack = { user_stack, user_stack };
    init_user_thread_stack(t, entry_fn, user_stack);
  } else {
    init_kernel_thread_stack(t, entry_fn);
  }

  if (current_running) {
    t->next = current_running;
    t->prev = current_running->prev;
    current_running->prev->next = t;
    current_running->prev = t;
  } else {
    t->next = t;
    t->prev = t;
    current_running = t;
  }

  return t;
}

void kernel_main() {
  init_serial();
  init_vga();
  init_gdt();
  pic_remap();
  init_interrupts();
  init_syscalls();

  serial_print("Hello World.\n");
  vga_write("Hello World\n");

  leave_critical();

  thread_create(test_writes, true);

  while (1) {
    asm volatile ("hlt");
  }
}
