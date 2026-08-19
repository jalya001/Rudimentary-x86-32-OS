#include "kernel.hpp"
#include "gdt.hpp"
#include "scheduler.hpp"
#include "entry.hpp"

static inline void halt() {
  while (1) {
    asm volatile("hlt");
  }
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
  tss.esp0 = current_running->kernel_stack.base + STACK_SIZE;
}

void exit() {
  current_running->state = EXITED;
  scheduler_entry();
}