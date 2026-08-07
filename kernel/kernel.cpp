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
Thread* threads[THREAD_LIMIT];

/*
uint32_t stack_counter = 0;

uint32_t allocate_stack() { // temporary until proper stack manager is made
  uintptr_t base = STACKS_START + stack_counter * STACK_SIZE;
  Stack stack = { base, base };
  stack_counter++;
  return base;
}

uint32_t thread_create(enum ThreadType type) {
  uint32_t tid = tid_allocator.allocate_id();
  if (tid < 0) return tid;
  thread[tid-1] = { tid, TaskState.READY, allocate_stack(), type == ThreadType.USER ? allocate_stack() : 0 };
  if (ThreadType.USER) {
    buffer_zero(thread[tid-1].stack.sp - 16, 16); // 16 is context size?
    *(thread[tid-1].stack.sp - 16) = &dummy_point; // when the context gets popped we want it to
  }
  return tid;
}

uint32_t process_create() {
  uint32_t pid = pid_allocator.allocate_id();
  if (pid < 0) return pid;
  thread[pid-1] = { pid, TaskState.READY, 0, thread_create() };
  return pid;
}

*/

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

  // create a trampoline process before it goes to the first process
  // ...

  // test
  test();

  while (1) {
    asm volatile ("hlt");
  }
}
