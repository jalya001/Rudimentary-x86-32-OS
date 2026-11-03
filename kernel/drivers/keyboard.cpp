#include "keyboard.hpp"
#include "../print.hpp"

#define KEYBOARD_DATA_PORT 0x60

// Scancode set 1 - key-down codes only (bit 7 set = key-up, ignore)
static const char scancode_map[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', // 9
    '9', '0', '-', '=', '\b', // Backspace
    '\t', // Tab
    'q', 'w', 'e', 'r', // 19
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', // Enter key
    0, // 29 - Control
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'', '`',
    0, // Left shift
    '\\',
    'z', 'x', 'c', 'v', 'b',
    'n', // 39
    'm',
    ',', '.', '/',
    0, // Right shift
    '*',
    0, // Alt
    ' ', // Space bar
};

#define KBD_BUFFER_SIZE 64
static char kbd_buffer[KBD_BUFFER_SIZE];
static volatile int kbd_head = 0;
static volatile int kbd_tail = 0;

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    asm volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

extern "C" void keyboard_handler_c() {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    if (scancode & 0x80) return; // key-up, ignore

    char c = (scancode < 128) ? scancode_map[scancode] : 0;
    if (!c) return; // unmapped key, ignore

    int next = (kbd_head + 1) % KBD_BUFFER_SIZE;
    if (next != kbd_tail) { // drop if full
        kbd_buffer[kbd_head] = c;
        kbd_head = next;
    }

    // Send EOI to PIC (Programmable Interrupt Controller)
    asm volatile("outb %0, %1" :: "a"((uint8_t)0x20), "Nd"((uint16_t)0x20));
}

// Returns -1 if buffer is empty, otherwise returns the character
int kbd_getchar() {
    if (kbd_head == kbd_tail) return -1; // buffer empty
    
    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUFFER_SIZE;
    return (int)c;
}