#include <u.h>
#include <sys/stat.h>
#include <draw.h>

vlong _drawflength(int fd) {
	struct stat s;

	if (fstat(fd, &s) < 0) {
		return -1;
	}
	return s.st_size;
}
