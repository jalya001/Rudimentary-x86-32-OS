#include "gdt.hpp"
#include "kernel.hpp"

GDTEntry gdt[6];
GDTPointer gdt_ptr = { sizeof(gdt) - 1, (uint32_t)&gdt };
TSS tss = {};

// Helper that rearranges the order because the usual order is disjointed
void set_gdt_entry(GDTEntry *gdt, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
  gdt->base_low = base & 0xFFFF;
  gdt->base_middle = (base >> 16) & 0xFF;
  gdt->base_high = (base >> 24) & 0xFF;
  gdt->limit_low = limit & 0xFFFF;
  gdt->granularity = ((limit >> 16) & 0x0F);
  gdt->granularity |= flags & 0xF0;
  gdt->access = access;
}

void load_gdt() {
  asm volatile(
    "lgdt (%0)              \n"

    "ljmp $0x08, $1f        \n"

    "1:                     \n"
    "mov $0x10, %%ax        \n" // segments need to be set manually?
    "mov %%ax, %%ds         \n"
    "mov %%ax, %%es         \n"
    "mov %%ax, %%ss         \n"
    //"mov %%ax, %%fs         \n"
    //"mov %%ax, %%gs         \n"
    

    "mov $0x28, %%ax        \n"
    "ltr %%ax               \n"

    :
    : "r"(&gdt_ptr)
    : "ax", "memory"
  );
}

void init_gdt() {
  // Null descriptor
  set_gdt_entry(&gdt[0], 0, 0, 0, 0);

  // Kernel code
  set_gdt_entry(&gdt[1], 0, 0xFFFFF, 0x9A, 0xC0);

  // Kernel data
  set_gdt_entry(&gdt[2], 0, 0xFFFFF, 0x92, 0xC0);

  // User code
  set_gdt_entry(&gdt[3], 0, 0xFFFFF, 0xFA, 0xC0);

  // User data
  set_gdt_entry(&gdt[4], 0, 0xFFFFF, 0xF2, 0xC0);

  // TSS
  // The below fields are the only ones that need to be set in software 
  tss.esp0 = STACKS_START + STACK_SIZE; // bootstrap value. replace with setting it at the dummy process and scheduler setting it automatically?
  tss.ss0 = 0x10;
  tss.iopb = sizeof(TSS);
  set_gdt_entry(&gdt[5], (uint32_t)&tss, sizeof(TSS) - 1, 0x89, 0x00);
  
  load_gdt();
}