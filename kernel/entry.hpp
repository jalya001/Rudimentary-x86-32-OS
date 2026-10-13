#pragma once

#include <minstd/types.hpp>

extern "C" {
  void enter_critical();
  void leave_critical();
  void scheduler_entry();
  void direct_scheduler_entry();
  void interrupt_entry();
  void syscall_entry();
  void timer_handler();
  void kthread_trampoline();
  void uthread_trampoline();
  void startup_trampoline();
}

extern "C" {
  extern uint32_t critical_depth;
  extern uint32_t critical_saved_flags;
}
