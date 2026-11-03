#include <syslib.hpp>

extern "C" void _start() {
    call_write(2, "Hello from userspace\n", nullptr);
    call_exit();
}
