#pragma once

#include "kernel.hpp"

enum {
  UNLOCKED,
  LOCKED
};

struct Lock {
  int status = UNLOCKED;
  Thread *queue = nullptr;

  void acquire();
  void release();
};

struct Condition {
  Thread *queue = nullptr;

  void wait(Lock *l);
  void signal();
  void broadcast();
};

struct Semaphore {
  int slots = 1;
  Thread *queue = nullptr;

  void up();
  void down();
};

struct Barrier {
  int counter = 1;
  Thread *queue = nullptr;

  void wait();
};