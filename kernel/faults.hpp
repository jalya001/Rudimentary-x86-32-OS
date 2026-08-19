#pragma once

extern "C" {
  void general_protection_handler();
  void double_fault_handler();
  void default_interrupt_handler();
  void page_fault_handler();
}