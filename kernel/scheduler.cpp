#include "kernel.hpp"
#include "gdt.hpp"
#include "scheduler.hpp"
#include "entry.hpp"
#include "print.hpp"
#include "paging.hpp"
#include "structs/controlblock.hpp"
#include "id.hpp"

void r0_yield() {
  direct_scheduler_entry();
}

Thread *pop_ready_queue() {
  Thread *to_remove;

  if (current_running->next == current_running) halt();
  to_remove = current_running;
  current_running = current_running->next;
  to_remove->prev->next = to_remove->next;
  to_remove->next->prev = to_remove->prev;
  to_remove->next = nullptr;
  to_remove->prev = nullptr;

  return to_remove;
}

uint32_t scheduler(uint32_t old_esp) {
  static tcb_t *zombie = 0;
  
  current_running->kernel_stack.sp = old_esp;
  
  /* Reap whichever thread exited during the *previous* switch. By now
   * we're running on a different stack than it was, so its memory is
   * safe to hand back to the allocator. (Freeing it any earlier, e.g.
   * right when it exits, would mean freeing the very stack we're still
   * executing on). 
   */
  if (zombie) {
    thread_reap(zombie);
    zombie = 0;
  }

  switch (current_running->state) {
    case READY:
      current_running = current_running->next;
      break;
    case EXITED_NO_ZOMBIE:
    case BLOCKED: // removes from ready queue because it's in a blocked queue now
      pop_ready_queue();
      break;
    case EXITED:
      zombie = pop_ready_queue();
      break;
    case RUNNING:
    default:
      break;
  }
  
  current_running->state = READY;
  tss.esp0 = current_running->kernel_stack.bottom + STACK_SIZE;

  // Switch page directory if the thread has one
  if (current_running->page_directory) {
    asm volatile("mov %0, %%cr3" : : "r"(current_running->page_directory) : "memory");
  }

  return current_running->kernel_stack.sp;
}

void r0_exit() {
  tid_allocator->close(current_running->tid);
  while (true) direct_scheduler_entry();
}

void r0_exit_no_zombie() {
  current_running->state = EXITED_NO_ZOMBIE;
  direct_scheduler_entry();
}

extern "C" void r3_exit() { // For when interrupts already get disabled
  current_running->state = EXITED;
  scheduler_entry();
}

void block(tcb_t **q) {
  current_running->state = BLOCKED;

  if (*q == nullptr) {
    *q = current_running;
  } else {
    tcb_t *t = *q;
    while (t->next != nullptr) t = t->next;
    t->next = current_running;
  }

  scheduler_entry();
}

void unblock(tcb_t **q) {
  if (*q == nullptr) kprintf("ERROR: unblocking a nullpointer\n");

  tcb_t *t = *q;
  *q = (*q)->next;

  t->state = READY;
  t->next = current_running->next;
  t->prev = current_running;
  current_running->next->prev = t;
  current_running->next = t;
}
