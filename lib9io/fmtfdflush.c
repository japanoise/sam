/* Copyright (c) 2002-2006 Lucent Technologies; see LICENSE */
#include <stdint.h>
#include <u.h>
#include <stdarg.h>
#include <unistd.h>
#include "fmt.h"
#include "fmtdef.h"

/*
 * generic routine for flushing a formatting buffer
 * to a file descriptor
 */
int __fmtFdFlush(Fmt *f) {
	int n;

	n = (char *)f->to - (char *)f->start;
	if (n && write((uintptr_t)f->farg, f->start, n) != n) {
		return 0;
	}
	f->to = f->start;
	return 1;
}
