#pragma once

#include <minstd/types.hpp>

/* The plan as of now is to simply implement it as a busy-wait. */

void sleep(uint32_t msecs);