#pragma once

extern "C" {
  void enter_critical();
  void leave_critical();
  void scheduler_entry();
  void interrupt_entry();
  void syscall_entry();
  void timer_handler();
}