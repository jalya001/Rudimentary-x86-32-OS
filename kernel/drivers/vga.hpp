#pragma once

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_ADDRESS 0xB8000

void init_vga(void);
void vga_clear(void);
void vga_clear_region(int minx, int miny, int maxx, int maxy);
void vga_set_cursor(int x, int y);
void vga_get_cursor(int *x, int *y);
void vga_put_char(char c);
void vga_write(const char *str);
void vga_scroll(int lines);