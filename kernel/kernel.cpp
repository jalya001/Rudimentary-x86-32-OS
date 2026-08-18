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

StackAllocator stack_allocator;
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
template <bool User> // using a bool is rather opaque
tcb_t *thread_create(void (*entry_fn)()) {
  uint32_t tid = tid_allocator.allocate_id();
  if (tid == (uint32_t) - 1) return 0;

  tcb_t *t = &thread_pool[tid - 1];
  *t = {};
  t->tid = tid;
  t->state = READY;
  t->kernel_stack = stack_allocator.allocate();

 /* --- thread bootstrap ---
  *
  * A thread that's never run yet has no saved registers for scheduler_entry
  * to restore. So we pre-build its kernel stack to look like it just called
  * scheduler_entry and is about to `ret`, except the "return address"
  * points at a trampoline instead of a real saved code. Field order here
  * mirrors what scheduler_entry itself pushes/pops.
  */
  uint8_t *top = (uint8_t *)t->kernel_stack.sp;
  top -= sizeof(SwitchFrame);
  auto *frame = (SwitchFrame *)top;
  t->kernel_stack.sp = (uint32_t)(uintptr_t)frame;
  *frame = {};
  frame->gs = 0x10;
  frame->fs = 0x10;
  frame->es = 0x10;
  frame->ds = 0x10;
  frame->ebx = (uint32_t)(uintptr_t)entry_fn; // both trampolines read this

  if constexpr (User) {
    t->user_stack = stack_allocator.allocate();

    frame->return_eip = (uint32_t)(uintptr_t)uthread_trampoline;
    frame->esi = (uint32_t)t->user_stack.sp; // uthread_trampoline reads this 
  } else {
    frame->return_eip = (uint32_t)(uintptr_t)kthread_trampoline;
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


  /* A placeholder TCB representing kernel_main's own execution context, so
   * switching away from it doesn't collide with the first real thread.
   */
  static Thread boot_thread = {};
  boot_thread.tid = 0;
  boot_thread.state = READY;
  boot_thread.kernel_stack = stack_allocator.allocate();
  boot_thread.next = &boot_thread;
  boot_thread.prev = &boot_thread;
  current_running = &boot_thread;

  thread_create<true>(test_writes);
  thread_create<true>(test_writes_2);
  thread_create<true>(test_writes_3);

  yield();    // hand off to the first real thread

  while (1) {
    asm volatile ("hlt");
  }
}
