#pragma once

/*
 * load_program - find a user program by name in the on-disk program directory
 *                and launch it as a new user (ring-3) thread.
 * 
 * Returns 0 on success, -1 on any error (disk read failure, program not found,
 * out of memory, thread limit reached, etc.).
 */
int load_program(const char *name);