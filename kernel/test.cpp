#include <syslib.hpp>
#include <minstd/types.hpp>
#include "kernel.hpp"
#include "drivers/serial.hpp"

struct Registers {
  uint32_t gs;
  uint32_t fs;
  uint32_t es;
  uint32_t ds;

  uint32_t edi;
  uint32_t esi;
  uint32_t ebp;
  uint32_t esp;
  uint32_t ebx;
  uint32_t edx;
  uint32_t ecx;
  uint32_t eax;
};

struct InterruptFrame {
  uint32_t eip;
  uint32_t cs;
  uint32_t eflags;
  uint32_t esp;
  uint32_t ss;
};

constexpr uint32_t USER_CS = 0x1B;
constexpr uint32_t USER_SS = 0x23;
constexpr uint32_t DEFAULT_EFLAGS = 0x202;

void create_user_context(InterruptFrame *frame, uint32_t entry, uint32_t user_stack) {
  frame->eip = entry;
  frame->cs = USER_CS;
  frame->eflags = DEFAULT_EFLAGS;
  frame->esp = user_stack;
  frame->ss = USER_SS;
}

void test_writes() {
  call_write("HELLO THIS IS A TEST\n");
  call_write("SECOND CALL\n");
  call_write("Third\n");
  call_write("FOURTHHHHHHHHHH\n");
}

void test() {
  Thread dthread = { 0, READY, {STACKS_START + STACK_SIZE, STACKS_START + STACK_SIZE}, {STACKS_START + STACK_SIZE + STACK_SIZE, STACKS_START + STACK_SIZE + STACK_SIZE}, 0, 0 };
  //Process process = {}; // not needed yet

  uint8_t* top = (uint8_t *)STACKS_START + STACK_SIZE;
  top -= sizeof(InterruptFrame);
  auto* frame = (InterruptFrame *)top;
  create_user_context(frame, (uint32_t)test_writes, STACKS_START + STACK_SIZE + STACK_SIZE);
  top -= sizeof(Registers);
  Registers *regs = (Registers *)top;
  *regs = {};
  regs->ds = USER_SS;
  regs->es = USER_SS;
  regs->fs = USER_SS;
  regs->gs = USER_SS;
  dthread.kernel_stack.sp = (uintptr_t)regs;
  
  fd_write(1, "check 1\n");

  asm volatile(
    "mov %0, %%esp        \n"

    "pop %%eax            \n"
    "mov %%ax, %%gs       \n"

    "pop %%eax            \n"
    "mov %%ax, %%fs       \n"

    "pop %%eax            \n"
    "mov %%ax, %%es       \n"

    "pop %%eax            \n"
    "mov %%ax, %%ds       \n"

    "popa                 \n"
    "iret                 \n"
    :
    : "r"(dthread.kernel_stack.sp)
    : "eax", "memory"
  );

  __builtin_unreachable();
}