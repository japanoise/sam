/* Copyright (c) 2002-2006 Lucent Technologies; see LICENSE */
#include <stdlib.h>
#include <stdarg.h>
#include "fmt.h"
#include "fmtdef.h"

int vsnprint(char *buf, int len, char *fmt, va_list args) {
	Fmt f;

	if (len <= 0) {
		return -1;
	}
	f.runes = 0;
	f.start = buf;
	f.to = buf;
	f.stop = buf + len - 1;
	f.flush = 0;
	f.farg = NULL;
	f.nfmt = 0;
	VA_COPY(f.args, args);
	fmtlocaleinit(&f, NULL, NULL, NULL);
	dofmt(&f, fmt);
	VA_END(f.args);
	*(char *)f.to = '\0';
	return (char *)f.to - buf;
}
