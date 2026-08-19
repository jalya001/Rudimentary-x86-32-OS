#include <syslib.hpp>
#include <sleep.hpp>

void test_writes() {
  call_write("Test 1 begin\n");

  while (1) { sleep(500); call_write("Test 1 testing 111111111\n"); sleep(500); }
  //while (1) { call_yield(); }
}

void test_writes_2() {
  call_write("Test 2 begin\n");

  while (1) { sleep(500); call_write("Test 2 testing 2222\n"); sleep(500); }
}

void test_writes_3() {
  call_write("Test 3 begin\n");

  while (1) { sleep(500); call_write("Test 3 testing 3\n"); sleep(500); }
}

void test_exiter() {
  call_write("Exiter: running once, exiting now\n");
  call_exit();
}

void test_after_exit() {
  call_write("AfterExit: I'm alive — reused a freed stack slot\n");
  while (1) { call_yield(); }
}

void noop_thread() {
  while (1) { call_yield(); }
}

void stress_a() { 
  while (1) { 
    call_write("A\n"); 
  } 
}

void stress_b() { 
  while (1) { 
    call_write("B\n"); 
  } 
}