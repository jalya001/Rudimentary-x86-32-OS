#pragma once

#include "kernel.hpp"

void r0_yield(void);
extern "C" void r0_exit(void);
void scheduler(void);
void dispatch(void);

void r3_exit(void);