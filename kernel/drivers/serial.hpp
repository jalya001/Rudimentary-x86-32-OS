#pragma once

void init_serial(void);
void serial_print(const char* s);
void serial_print(char c);
void serial_print(const char* buf, int len);
void serial_print(int value);