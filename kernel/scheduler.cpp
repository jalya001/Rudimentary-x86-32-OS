#include "kernel.hpp"
#include "gdt.hpp"
#include "scheduler.hpp"
#include "entry.hpp"

static inline void halt() {
  asm volatile("hlt");
}

void yield() {
  enter_critical();
  scheduler_entry();
  leave_critical();
}

extern "C" uint32_t save_and_get_next_esp(uint32_t old_esp) {
  current_running->kernel_stack.sp = old_esp;
  scheduler();
  return current_running->kernel_stack.sp;
}

void scheduler() {
  tcb_t *to_remove;
  switch (current_running->state) {
    case READY:
      current_running = current_running->next;
      break;
    case BLOCKED:
    case EXITED:
      // cleanup of PCB is supposed to happen at process' exit call?
      if (current_running->next == current_running) halt();
      to_remove = current_running;
      current_running = current_running->next;
      to_remove->prev->next = to_remove->next;
      to_remove->next->prev = to_remove->prev;
      to_remove->next = 0;
      to_remove->prev = 0;
      break;
    case RUNNING:
    default:
      break;
  }
  
  current_running->state = READY;
  tss.esp0 = current_running->kernel_stack.sp;
}

void exit() {
  current_running->state = EXITED;
  scheduler_entry();
}
  
/* --- thread bootstrap ---
 *
 * A thread that's never run yet has no saved registers for scheduler_entry
 * to restore. So we pre-build its kernel stack to look like it just called
 * scheduler_entry and is about to `ret`, except the "return address"
 * points at a trampoline instead of a real saved code. Field order here
 * mirrors what scheduler_entry itself pushes/pops.
 */

struct SwitchFrame {
  uint32_t gs, fs, es, ds;
  uint32_t edi, esi, ebp, esp_unused, ebx, edx, ecx, eax;
  uint32_t return_eip;
};

void init_kernel_thread_stack(tcb_t *t, void (*entry_fn)()) {
  uint8_t *top = (uint8_t *)t->kernel_stack.base;
  top -= sizeof(SwitchFrame);
  auto *frame = (SwitchFrame *)top;
  *frame = {};
  frame->gs = 0x10;
  frame->fs = 0x10;
  frame->es = 0x10;
  frame->ds = 0x10;
  frame->ebx = (uint32_t)(uintptr_t)entry_fn;   // kthread_trampoline reads this
  frame->return_eip = (uint32_t)(uintptr_t)kthread_trampoline;
  t->kernel_stack.sp = (uint32_t)(uintptr_t)frame;
}

void init_user_thread_stack(tcb_t *t, void (*entry_fn)(), uintptr_t user_stack) {
  uint8_t *top = (uint8_t *)t->kernel_stack.base;
  top -= sizeof(SwitchFrame);
  auto *frame = (SwitchFrame *)top;
  *frame = {};
  frame->gs = 0x10;
  frame->fs = 0x10;
  frame->es = 0x10;
  frame->ds = 0x10;
  frame->ebx = (uint32_t)(uintptr_t)entry_fn;   // uthread_trampoline reads this 
  frame->esi = (uint32_t)user_stack;            // and this
  frame->return_eip = (uint32_t)(uintptr_t)uthread_trampoline;
  t->kernel_stack.sp = (uint32_t)(uintptr_t)frame;
}