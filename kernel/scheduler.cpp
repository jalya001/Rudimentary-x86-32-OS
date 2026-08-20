#include "kernel.hpp"
#include "gdt.hpp"
#include "scheduler.hpp"
#include "entry.hpp"

tcb_t *zombie = 0;

static inline void halt() {
  while (1) {
    asm volatile("hlt");
  }
}

void r0_yield() {
  direct_scheduler_entry();
}

extern "C" uint32_t save_and_get_next_esp(uint32_t old_esp) {
  current_running->kernel_stack.sp = old_esp;
  
  /* Reapp whichever thread exited during the *previous* switch. By now
   * we're running on a different stack than it was, so its memory is
   * safe to hand back to the allocator. (Freeing it any earlier, e.g.
   * right when it exits, would mean freeing the very stack we're still
   * executing on). 
   */
  if (zombie) {
    stack_allocator.free(zombie->kernel_stack);
    if (zombie->user_stack.base) stack_allocator.free(zombie->user_stack);
    zombie = 0;
  }
  
  scheduler();
  return current_running->kernel_stack.sp;
}

void scheduler() {
  tcb_t *to_remove;
  switch (current_running->state) {
    case READY:
      current_running = current_running->next;
      break;
    case EXITED:
      if (current_running->next == current_running) halt();
      to_remove = current_running;
      current_running = current_running->next;
      to_remove->prev->next = to_remove->next;
      to_remove->next->prev = to_remove->prev;
      to_remove->next = 0;
      to_remove->prev = 0;
      break;
    case BLOCKED:
        /* TODO: a blocked thread is still alive and needs a wait queue to
         * live on, not deletion. This falls out of scope until 
         * synchronization primitives exist and something actually sets
         * BLOCKED. Leaving unhandled (falls through to default) rather than
         * silently doing the wrong thing.
         */
    case RUNNING:
    default:
      break;
  }
  
  current_running->state = READY;
  tss.esp0 = current_running->kernel_stack.base + STACK_SIZE;
}

void r0_exit() {
  current_running->state = EXITED;
  direct_scheduler_entry();
}

void r3_exit() { // For when interrupts already get disabled
  current_running->state = EXITED;
  scheduler_entry();
}