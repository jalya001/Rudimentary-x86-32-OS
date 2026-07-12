#include "vga.hpp"
#include "serial.hpp"

static short *screen = (short *)VGA_ADDRESS;
static char vga_color;

static char cursor_x;
static char cursor_y;

void init_vga(void) {
  vga_color = 0x07;

  vga_clear();

  cursor_x = 0;
  cursor_y = 0;
  vga_set_cursor(0, 0);
}

void vga_clear(void) {
  short blank = ((short)vga_color << 8) | ' ';
  for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) screen[i] = blank;
}

void vga_clear_region(int minx, int miny, int maxx, int maxy) {
  for (int y = miny; y < maxy; y++) {
    int offset = y * VGA_WIDTH;
    for (int x = minx; x < maxx; x++) screen[offset + x] = 0x0700;
  }
}

static void vga_move_hardware_cursor(void) {
  short pos = cursor_y * VGA_WIDTH + cursor_x;

  outb(0x3D4, 0x0F);
  outb(0x3D5, (char)(pos & 0xFF));

  outb(0x3D4, 0x0E);
  outb(0x3D5, (char)((pos >> 8) & 0xFF));
}

void vga_set_cursor(int x, int y) {
  if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT) return;
  cursor_x = x;
  cursor_y = y;
  vga_move_hardware_cursor();
}

void vga_get_cursor(int *x, int *y) {
  *x = cursor_x;
  *y = cursor_y;
}

void vga_put_char(char c) {
  if (c == '\n') {
    cursor_x = 0;
    cursor_y++;
  } else if (c == '\r') {
    cursor_x = 0;
  } else if (c == '\t') {
    cursor_x = (cursor_x + 8) & ~(8 - 1);
  } else {
    short entry = ((short)vga_color << 8) | c;
    screen[cursor_y * VGA_WIDTH + cursor_x] = entry;
    cursor_x++;

    // wrap-around
    if (cursor_x >= VGA_WIDTH) {
      cursor_x = 0;
      cursor_y++;
    }
  }

  // wrap-around
  if (cursor_y >= VGA_HEIGHT) cursor_y = 0;

  vga_move_hardware_cursor();
}

void vga_write(const char *str) {
  while (*str != '\0') {
    vga_put_char(*str);
    str++;
  }
}

void vga_scroll(int lines) {
  if (lines <= 0) return;
  short blank = ((short)vga_color << 8) | ' ';

  if (lines >= VGA_HEIGHT) {
    for (short *p = screen; p < screen + VGA_WIDTH * VGA_HEIGHT; p++) *p = blank;
    cursor_x = 0;
    cursor_y = 0;
    return;
  }

  // move the stuff upward
  // should have been memmove if we had it
  int rows_to_move = VGA_HEIGHT - lines;
  for (int y = 0; y < rows_to_move; y++) {
    short *dst = screen + y * VGA_WIDTH;
    short *src = screen + (y + lines) * VGA_WIDTH;
    for (int x = 0; x < VGA_WIDTH; x++) dst[x] = src[x];
  }

  // blank out the residue of stuff that was moved
  short *row = screen + rows_to_move * VGA_WIDTH;
  for (int y = 0; y < lines; y++) {
    for (int x = 0; x < VGA_WIDTH; x++) row[x] = blank;
    row += VGA_WIDTH;
  }

  cursor_y -= lines;
  if (cursor_y < 0) cursor_y = 0;
}