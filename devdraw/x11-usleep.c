#include <time.h>

int usleep(long microseconds) {
	struct timespec duration, remaining;
	duration.tv_sec = 0;
	duration.tv_nsec = microseconds * 1000L;

	return nanosleep(&duration, &remaining);
}
