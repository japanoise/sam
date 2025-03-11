/* Copyright (c) 2002-2006 Lucent Technologies; see LICENSE */
#include <stdarg.h>
#include "fmt.h"
#include "fmtdef.h"

/*
 * format a string into the output buffer
 * designed for formats which themselves call fmt,
 * but ignore any width flags
 */
int fmtprint(Fmt *f, char *fmt, ...) {
	va_list va;
	int     n;

	f->flags = 0;
	f->width = 0;
	f->prec = 0;
	va_copy(va, f->args);
	va_end(f->args);
	va_start(f->args, fmt);
	n = dofmt(f, fmt);
	va_end(f->args);
	f->flags = 0;
	f->width = 0;
	f->prec = 0;
	va_copy(f->args, va);
	va_end(va);
	if (n >= 0) {
		return 0;
	}
	return n;
}
