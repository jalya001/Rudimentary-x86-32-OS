#include "entry.hpp"
#include "faults.hpp"
#include "interrupts.hpp"

#define KERNEL_CS 0x08
#define INTERRUPT_GATE 0x8E

IDTE idt[256];
IDTPTR idtp;

void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t flags) {
  idt[num].offset_low = handler & 0xFFFF;
  idt[num].selector = selector;
  idt[num].reserved = 0;
  idt[num].flags = flags;
  idt[num].offset_high = handler >> 16;
}

void init_interrupts() {
  for (int i = 0; i < 256; i++) { idt_set_gate(i, (uint32_t)default_interrupt_handler, KERNEL_CS, INTERRUPT_GATE); }
  idt_set_gate(0x08, (uint32_t)double_fault_handler, KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(0x0D, (uint32_t)general_protection_handler, KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(32, (uint32_t)timer_handler, KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(0x80, (uint32_t)syscall_entry, KERNEL_CS, 0xEE);
  idt_set_gate(14, (uint32_t)page_fault_handler, KERNEL_CS, INTERRUPT_GATE);

  idtp.limit = sizeof(idt) - 1;
  idtp.base  = (uint32_t)&idt;
  asm volatile("lidt %0" : : "m"(idtp));
}