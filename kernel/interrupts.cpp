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
  idt_set_gate(0x00, (uint32_t)divide_error_handler,         KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(0x01, (uint32_t)debug_handler,                KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(0x02, (uint32_t)nmi_handler,                  KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(0x03, (uint32_t)breakpoint_handler,           KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(0x04, (uint32_t)overflow_handler,             KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(0x05, (uint32_t)bounds_handler,               KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(0x06, (uint32_t)invalid_opcode_handler,       KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(0x07, (uint32_t)device_not_available_handler, KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(0x08, (uint32_t)double_fault_handler,         KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(0x09, (uint32_t)coprocessor_segment_handler,  KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(0x0A, (uint32_t)invalid_tss_handler,          KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(0x0B, (uint32_t)segment_not_present_handler,  KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(0x0C, (uint32_t)stack_segment_handler,        KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(0x0D, (uint32_t)general_protection_handler,   KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(0x0E, (uint32_t)page_fault_handler,           KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(0x10, (uint32_t)x87_fpu_handler,              KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(0x11, (uint32_t)alignment_check_handler,      KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(0x12, (uint32_t)machine_check_handler,        KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(0x13, (uint32_t)simd_fpu_handler,             KERNEL_CS, INTERRUPT_GATE);

  idt_set_gate(32, (uint32_t)timer_handler, KERNEL_CS, INTERRUPT_GATE);
  idt_set_gate(0x80, (uint32_t)syscall_entry, KERNEL_CS, 0xEE);

  idtp.limit = sizeof(idt) - 1;
  idtp.base  = (uint32_t)&idt;
  asm volatile("lidt %0" : : "m"(idtp));
}