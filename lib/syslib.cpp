#include <minstd/types.hpp>
#include <common.hpp>
#include <syslib.hpp>

#define UNUSED 0

static inline int invoke_syscall(uint32_t syscall_id, uint32_t arg1, uint32_t arg2, uint32_t arg3) { // expand to 6 later
  int ret;
  asm volatile (
    "int $0x80"
    : "=a"(ret)
    : "a"(syscall_id),
      "b"(arg1),
      "c"(arg2),
      "d"(arg3)
    : "memory"
  );

  return ret;
}

void call_yield(void) {
  invoke_syscall(SYS_YIELD, UNUSED, UNUSED, UNUSED);
}

void call_exit(void) {
  invoke_syscall(SYS_EXIT, UNUSED, UNUSED, UNUSED);
}

void call_write(const char *msg) {
  invoke_syscall(SYS_WRITE, 1, (int)msg, UNUSED); // could go out of bounds and explode without a terminator, but eh
}