#include <syslib.hpp>

void test_writes() {
  call_write("HELLO THIS IS A TEST\n");
  call_write("SECOND CALL\n");
  call_write("Third\n");
  call_write("FOURTHHHHHHHHHH\n");
  while (1) { call_yield(); }
}