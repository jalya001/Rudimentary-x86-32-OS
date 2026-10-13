#include "kernel.hpp"
#include "print.hpp"
#include <minstd/types.hpp>
#include "drivers/serial.hpp"
#include "drivers/vga.hpp"

template <int fd>
inline static void putc_internal(char c) { // inline wtf
  if constexpr (fd == 1) {
    serial_print(c);
  } else if constexpr (fd == 2) {
    vga_put_char(c);
  } else if constexpr (fd == 3) {
    serial_print(c);
    vga_put_char(c);
  }
}

void fd_printf_internal(int fd, const char *fmt, uint32_t *args) {
  static const char* digits = "0123456789ABCDEF";

  constexpr void (*putc_table[])(char) = {
    nullptr,
    putc_internal<1>,
    putc_internal<2>,
    putc_internal<3>,
  };

  auto putc = putc_table[fd];

  auto print_number = [&](uint32_t value, unsigned int base) {
    char buffer[32];
    int i = 0;

    do {
      buffer[i++] = digits[value % base];
      value /= base;
    } while (value);

    while (i > 0) putc(buffer[--i]);
  };

  while (*fmt) {
    if (*fmt != '%') {
      putc(*fmt++);
      continue;
    }

    ++fmt;

    switch (*fmt) {
    case 'd':
      print_number(*args++, 10);
      break;

    case 'x':
      print_number(*args++, 16);
      break;

    case 's': {
      const char* s = (const char*)*args++;
      while (*s) putc(*s++);
      break;
    }

    case 'c':
      putc((char)*args++);
      break;

    case '%':
      putc('%');
      break;
    }

    ++fmt;
  }
}

void kprintf(const char* fmt, ...) {
  uint32_t *args = (uint32_t *)(&fmt + 1);
  fd_printf_internal(1, fmt, args);
}


void print_stack(uint32_t bytes) {
  uint32_t* esp;

  asm volatile("movl %%esp, %0" : "=r"(esp));

  for (uint32_t offset = 0; offset <= bytes; offset += 4) {
    uint32_t value = *(uint32_t*)((uint8_t*)esp + offset);

    kprintf("[ESP+0x%x] = 0x%x\n", offset, value);
  }
}

void print_eip() {
  uintptr_t eip;
  asm volatile(
      "call 1f\n"
      "1: pop %0\n"
      : "=r"(eip)
  );
  kprintf("EIP = 0x%x\n", eip);
}
