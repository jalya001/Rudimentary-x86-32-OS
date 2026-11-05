#include <syslib.hpp>
#include <common.hpp>

extern "C" void _start() {
    // Write
    int fd = call_fs_open("test.txt", FS_O_CREATE | FS_O_WRITE);
    call_fs_write(fd, (char*)"hello", 5);
    call_fs_close(fd);

    // Read back
    fd = call_fs_open("test.txt", FS_O_READ);
    char buf[8] = {};
    call_fs_read(fd, buf, 5);
    call_fs_close(fd);

    // Print result via exisiting write syscall
    call_write(3, buf, nullptr);    // should print "hello" to serial+VGA
    call_write(3, "\n", nullptr);

    call_exit();
}