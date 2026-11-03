#include <syslib.hpp>

static void shell_write(const char *s) {
    call_write(3, s, nullptr);
}

static void shell_putc(char c) {
    char buf[2] = {c, '\0'};
    call_write(2, buf, nullptr);
}

extern "C" void _start() {
    // Drain up to 256 startup garbage bytes, then move on regardless
    for (int i = 0; i < 256; i++) {
        if (call_getchar() == -1) break;
    }

    shell_write("Shell started\n");
    shell_write("Welcome to the Rudimentary OS shell!\n");
    shell_write("> ");

    char line[128];
    int len = 0;

    while (1) {
        int c = call_getchar();
        if (c == -1) {
            call_yield();
            continue;
        }

        if (c < 0x20 && c != '\n' && c != '\b') continue; // ignore non-printable characters except newline and backspace

        if (c == '\b') {
            if (len > 0) {
                len--;
                shell_write("\b \b");
            }
            continue;
        }

        shell_putc((char)c);

        if (c == '\n') {
            line[len] = '\0';
            // TODO: execute command in line
            len = 0;
            shell_write("> ");
            continue;
        }

        if (len < 127) line[len++] = (char)c;
    }
}