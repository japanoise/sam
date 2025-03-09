#include <time.h>
#include <stdint.h>
#include <inttypes.h>

#define BILLION 1000000000L

uint64_t nsec() {
	long int        ns;
	uint64_t        all;
	time_t          sec;
	struct timespec spec;

	clock_gettime(CLOCK_REALTIME, &spec);
	sec = spec.tv_sec;
	ns = spec.tv_nsec;

	return (uint64_t)sec * BILLION + (uint64_t)ns;
}
