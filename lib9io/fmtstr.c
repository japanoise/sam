/* Copyright (c) 2002-2006 Lucent Technologies; see LICENSE */
#include <stdarg.h>
#include "fmt.h"
#include "fmtdef.h"

char *fmtstrflush(Fmt *f) {
	if (f->start == NULL) {
		return NULL;
	}
	*(char *)f->to = '\0';
	f->to = f->start;
	return (char *)f->start;
}
