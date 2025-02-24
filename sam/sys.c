#include <errno.h>
#include <stdbool.h>

#include "sam.h"
#include <stdint.h>
#include <stdio.h>

static bool inerror = false;

#define ERRLEN 63

/*
 * A reasonable interface to the system calls
 */

void resetsys(void) { inerror = false; }

void syserror(char *a) {
	char buf[ERRLEN + 1] = {0};

	if (!inerror) {
		inerror = true;
		strncpy(buf, strerror(errno), ERRLEN);
		dprint("%s: ", a);
		error_s(Eio, buf);
	}
}

int Read(FILE *f, void *a, int n) {
	if (read(fileno(f), (char *)a, n) != n) {
		if (lastfile) {
			lastfile->rescuing = 1;
		}
		if (downloaded) {
			fprintf(stderr, "read error: %s\n", strerror(errno));
		}
		rescue();
		exit(EXIT_FAILURE);
	}
	return n;
}

int Write(FILE *f, void *a, int n) {
	int m;

	if ((m = write(fileno(f), (char *)a, n)) != n) {
		syserror("write");
	}
	return m;
}

void Seek(FILE *f, int64_t n, int w) {
	if (fseek(f, n, w) == -1) {
		syserror("seek");
	}
}
