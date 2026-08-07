#include <minstd/types.hpp>

// These are the only TSS fields that matter for us
#define TSS_ESP0 0x04
#define TSS_SS0 0x08
#define TSS_IOPB 0x66

struct GDTEntry {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_middle;
  uint8_t access;
  uint8_t granularity;
  uint8_t base_high;
} __attribute__((packed));

struct GDTPointer {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));

struct TSS {
  uint8_t irrelevant0[4];
  uint32_t esp0;
  uint16_t ss0;
  uint8_t irrelevant1[92];
  uint16_t iopb;
} __attribute__((packed));

void init_gdt();

extern TSS tss;