#pragma once
#include <minstd/types.hpp>

extern "C" void keyboard_handler_c();
int kbd_getchar();  // -1 if buffer empty