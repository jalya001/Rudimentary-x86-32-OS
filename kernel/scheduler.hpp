#pragma once

#include "kernel.hpp"

void yield(void);
void scheduler(void);
void dispatch(void);
void exit(void);

void init_kernel_thread_stack(tcb_t *t, void (*entry_fn)());
void init_user_thread_stack(tcb_t *t, void (*entry_fn)(), uintptr_t user_stack);