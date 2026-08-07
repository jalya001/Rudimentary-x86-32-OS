#include <minstd/types.hpp>
#include "drivers/serial.hpp"

#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01

#define ICW4_8086 0x01

void pic_remap() {
  uint8_t master_mask = 0xFF;
  uint8_t slave_mask  = 0xFF;

  master_mask = inb(PIC1_DATA);
  slave_mask  = inb(PIC2_DATA);

  outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
  outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);

  outb(PIC1_DATA, 0x20); // IRQ0-7 -> 0x20-0x27
  outb(PIC2_DATA, 0x28); // IRQ8-15 -> 0x28-0x2F

  outb(PIC1_DATA, 0x04);
  outb(PIC2_DATA, 0x02);

  outb(PIC1_DATA, ICW4_8086);
  outb(PIC2_DATA, ICW4_8086);

  outb(PIC1_DATA, master_mask);
  outb(PIC2_DATA, slave_mask);
}