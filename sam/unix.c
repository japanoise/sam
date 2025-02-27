/* Copyright (c) 1998 Lucent Technologies - All rights reserved. */
#include "sam.h"
#include "utf.h"
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>

#ifdef NEEDVARARG
#include <varargs.h>
#else
#include <stdarg.h>
#endif

Rune samname[] = {'~', '~', 's', 'a', 'm', '~', '~', 0};

static Rune l1[] = {'{', '[', '(', '<', 0253, 0};
static Rune l2[] = {'\n', 0};
static Rune l3[] = {'\'', '"', '`', 0};
Rune       *left[] = {l1, l2, l3, 0};

static Rune r1[] = {'}', ']', ')', '>', 0273, 0};
static Rune r2[] = {'\n', 0};
static Rune r3[] = {'\'', '"', '`', 0};
Rune       *right[] = {r1, r2, r3, 0};

void print_ss(char *s, String *a, String *b) {
	char *ap, *bp, *cp;
	Rune *rp;

	ap = emalloc(a->n + 1);
	for (cp = ap, rp = a->s; *rp; rp++) {
		cp += runetochar(cp, rp);
	}
	*cp = 0;
	bp = emalloc(b->n + 1);
	for (cp = bp, rp = b->s; *rp; rp++) {
		cp += runetochar(cp, rp);
	}
	*cp = 0;
	dprint("?warning: %s `%.*s' and `%.*s'\n", s, a->n, ap, b->n, bp);
	free(ap);
	free(bp);
}

void print_s(char *s, String *a) {
	char *ap, *cp;
	Rune *rp;

	ap = emalloc(a->n + 1);
	for (cp = ap, rp = a->s; *rp; rp++) {
		cp += runetochar(cp, rp);
	}
	*cp = 0;
	dprint("?warning: %s `%.*s'\n", s, a->n, ap);
	free(ap);
}

int statfile(char *name, uint64_t *dev, uint64_t *id, int64_t *time,
	     int64_t *length, int64_t *appendonly) {
	struct stat dirb;

	if (stat(name, &dirb) == -1) {
		return -1;
	}
	if (dev) {
		*dev = dirb.st_dev;
	}
	if (id) {
		*id = dirb.st_ino;
	}
	if (time) {
		*time = dirb.st_mtime;
	}
	if (length) {
		*length = dirb.st_size;
	}
	if (appendonly) {
		*appendonly = 0;
	}
	return 1;
}

int statfd(int fd, uint64_t *dev, uint64_t *id, int64_t *time, int64_t *length,
	   int64_t *appendonly) {
	struct stat dirb;

	if (fstat(fd, &dirb) == -1) {
		return -1;
	}
	if (dev) {
		*dev = dirb.st_dev;
	}
	if (id) {
		*id = dirb.st_ino;
	}
	if (time) {
		*time = dirb.st_mtime;
	}
	if (length) {
		*length = dirb.st_size;
	}
	if (appendonly) {
		*appendonly = 0;
	}
	return 1;
}

int newtmp(void) {
	FILE *f = tmpfile();
	if (f) {
		return fileno(f);
	}
	panic("could not create tempfile!");
	return -1;
}

void statehome(char *buf, char *filename) {
	int   dir_result = 0;
	char *xdg_state_home = getenv("XDG_STATE_HOME");

	if (xdg_state_home) {
		snprintf(buf, PATH_MAX, "%s/sam", xdg_state_home);
		mkdir_p(buf, 0755);
		snprintf(buf, PATH_MAX, "%s/sam/%s", xdg_state_home, filename);
	} else {
		char *home = getenv("HOME");
		snprintf(buf, PATH_MAX, "%s/.local/state/sam",
			 home ? home : "/tmp");
		mkdir_p(buf, 0755);
		snprintf(buf, PATH_MAX, "%s/.local/state/sam/%s",
			 home ? home : "/tmp", filename);
	}
}

void samerr(char *buf) { statehome(buf, "sam.err"); }

void samsave(char *buf) { statehome(buf, "sam.save"); }

int waitfor(int pid) {
	int wm;
	int rpid;

	do
		;
	while ((rpid = wait(&wm)) != pid && rpid != -1);
	return (WEXITSTATUS(wm));
}

void *emalloc(uint64_t n) {
	void *p = calloc(1, n < sizeof(int) ? sizeof(int) : n);
	if (!p) {
		panic("malloc failed");
	}
	return p;
}

void *erealloc(void *p, uint64_t n) {
	p = realloc(p, n);
	if (!p) {
		panic("realloc fails");
	}
	return p;
}

void dprint(char *z, ...) {
	char    buf[BLOCKSIZE + 1] = {0};
	va_list args;

	va_start(args, z);
	vsnprintf(buf, BLOCKSIZE, z, args);
	va_end(args);
	termwrite(buf);
}

int mkdir_p(const char *path, mode_t mode) {
	struct stat buf;

	if (stat(path, &buf) == 0) {
		if (S_ISDIR(buf.st_mode)) {
			return 0;
		}
		panic(
		    "mkdir_p: path to directory exists but is not a directory");
	}

	char   tmp[PATH_MAX];
	char  *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp), "%s", path);
	len = strlen(tmp);

	if (tmp[len - 1] == '/') {
		tmp[len - 1] = 0;
	}

	for (p = tmp + 1; *p; p++) {
		if (*p == '/') {
			*p = 0;
			mkdir(tmp, mode);
			*p = '/';
		}
	}

	mkdir(tmp, mode);
	return 0;
}
