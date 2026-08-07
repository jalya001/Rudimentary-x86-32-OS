#include <sleep.hpp>
#include <minstd/types.hpp>

void sleep(uint32_t msecs) { // simply busywait for now. will depend on timer interrupt later or something
  volatile uint32_t burn = 124214;
	while (msecs != 0) {
		burn = burn/msecs;
		burn = burn * (burn + 21321);
		burn = burn/12411;
		burn = burn + 21373;
		msecs--;
	}
}