#pragma once

#include "structs/controlblock.hpp"

void r0_yield(void);
extern "C" void r0_exit(void);
void r0_exit_no_zombie(void);
extern "C" uint32_t scheduler(uint32_t old_esp);

extern "C" void r3_exit(void);

void block(tcb_t **q);
void unblock(tcb_t **q);