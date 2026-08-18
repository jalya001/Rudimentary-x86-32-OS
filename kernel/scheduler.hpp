#pragma once

#include "kernel.hpp"

void yield(void);
void scheduler(void);
void dispatch(void);

extern "C" void exit(void);