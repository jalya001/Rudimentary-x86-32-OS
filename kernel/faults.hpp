#pragma once

extern "C" {
  void divide_error_handler();
  void debug_handler();
  void nmi_handler();
  void breakpoint_handler();
  void overflow_handler();
  void bounds_handler();
  void invalid_opcode_handler();
  void device_not_available_handler();
  void double_fault_handler();
  void coprocessor_segment_handler();
  void invalid_tss_handler();
  void segment_not_present_handler();
  void stack_segment_handler();
  void general_protection_handler();
  void page_fault_handler();
  void x87_fpu_handler();
  void alignment_check_handler();
  void machine_check_handler();
  void simd_fpu_handler();
  void default_interrupt_handler();
}