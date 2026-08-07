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
      current_running = current_running->next;
      to_remove = current_running;
      to_remove->prev->next = current_running;
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
  
