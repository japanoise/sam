/* Copyright (c) 2002-2006 Lucent Technologies; see LICENSE */
/*
 * Plan 9 port version must include libc.h in order to
 * get Plan 9 debugging malloc, which sometimes returns
 * different pointers than the standard malloc.
 */
#ifdef PLAN9PORT
#include <u.h>
#include <libc.h>
#include "fmtdef.h"
#else
#include <stdlib.h>
#include <string.h>
#include "fmt.h"
#include "fmtdef.h"
#endif

static int fmtStrFlush(Fmt *f) {
	char *s;
	int   n;

	if (f->start == NULL) {
		return 0;
	}
	n = (uintptr_t)f->farg;
	n *= 2;
	s = (char *)f->start;
	f->start = realloc(s, n);
	if (f->start == NULL) {
		f->farg = NULL;
		f->to = NULL;
		f->stop = NULL;
		free(s);
		return 0;
	}
	f->farg = (void *)(uintptr_t)n;
	f->to = (char *)f->start + ((char *)f->to - s);
	f->stop = (char *)f->start + n - 1;
	return 1;
}

int fmtstrinit(Fmt *f) {
	int n;

	memset(f, 0, sizeof *f);
	f->runes = 0;
	n = 32;
	f->start = malloc(n);
	if (f->start == NULL) {
		return -1;
	}
	f->to = f->start;
	f->stop = (char *)f->start + n - 1;
	f->flush = fmtStrFlush;
	f->farg = (void *)(uintptr_t)n;
	f->nfmt = 0;
	fmtlocaleinit(f, NULL, NULL, NULL);
	return 0;
}

/*
 * print into an allocated string buffer
 */
char *vsmprint(char *fmt, va_list args) {
	Fmt f;
	int n;

	if (fmtstrinit(&f) < 0) {
		return NULL;
	}
	VA_COPY(f.args, args);
	n = dofmt(&f, fmt);
	VA_END(f.args);
	if (n < 0) {
		free(f.start);
		return NULL;
	}
	return fmtstrflush(&f);
}
