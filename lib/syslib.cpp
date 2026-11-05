#include <minstd/types.hpp>
#include <common.hpp>
#include <syslib.hpp>

#define UNUSED 0

static inline int invoke_syscall(uint32_t syscall_id, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
  int ret;

  asm volatile (
      "int $0x80"
      : "=a"(ret)
      : "a"(syscall_id),
        "b"(arg1),
        "c"(arg2),
        "d"(arg3),
        "S"(arg4),
        "D"(arg5)
      : "memory"
  );

  return ret;
}

void call_yield(void) {
  invoke_syscall(SYS_YIELD, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED);
}

void call_exit(void) {
  invoke_syscall(SYS_EXIT, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED);
}

void call_exec(const char *name) {
  invoke_syscall(SYS_EXEC, (uint32_t)name, UNUSED, UNUSED, UNUSED, UNUSED);
}

int call_getchar() {
  return invoke_syscall(SYS_GETCHAR, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED);
}

void call_write(int fd, const char *fmt, uint32_t *args) {
  invoke_syscall(SYS_WRITE, fd, (uint32_t)fmt, (uint32_t)args, UNUSED, UNUSED); // could go out of bounds and explode without a terminator, but eh
}
int call_fs_open(const char *filename, int mode) {
  return invoke_syscall(SYS_FS_OPEN, (uint32_t)filename, (uint32_t)mode, UNUSED, UNUSED, UNUSED);
}

int call_fs_close(int fd) {
  return invoke_syscall(SYS_FS_CLOSE, (uint32_t)fd, UNUSED, UNUSED, UNUSED, UNUSED);
}

int call_fs_read(int fd, char *buffer, int size) {
  return invoke_syscall(SYS_FS_READ, (uint32_t)fd, (uint32_t)buffer, (uint32_t)size, UNUSED, UNUSED);
}

int call_fs_write(int fd, char *buffer, int size) {
  return invoke_syscall(SYS_FS_WRITE, (uint32_t)fd, (uint32_t)buffer, (uint32_t)size, UNUSED, UNUSED);
}

int call_fs_lseek(int fd, int offset, int whence) {
  return invoke_syscall(SYS_FS_LSEEK, (uint32_t)fd, (uint32_t)offset, (uint32_t)whence, UNUSED, UNUSED);
}

int call_fs_link(char *linkname, char *filename) {
  (void)linkname; (void)filename;
  return -1;  // not implemented
}

int call_fs_unlink(const char *linkname) {
  return invoke_syscall(SYS_FS_UNLINK, (uint32_t)linkname, UNUSED, UNUSED, UNUSED, UNUSED);
}

int call_fs_stat(int fd, char *buffer) {
  return invoke_syscall(SYS_FS_STAT, (uint32_t)fd, (uint32_t)buffer, UNUSED, UNUSED, UNUSED);
}
