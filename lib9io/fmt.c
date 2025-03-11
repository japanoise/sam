/* Copyright (c) 2002-2006 Lucent Technologies; see LICENSE */
#include <stdarg.h>

/*
 * As of 2020, older systems like RHEL 6 and AIX still do not have C11 atomics.
 * On those systems, make the code use volatile int accesses and hope for the
 * best. (Most uses of fmtinstall are not actually racing with calls to print
 * that lookup formats. The code used volatile here for years without too many
 * problems, even though that's technically racy. A mutex is not OK, because we
 * want to be able to call print from signal handlers.)
 *
 * RHEL is using an old GCC (atomics were added in GCC 4.9).
 * AIX is using its own IBM compiler (XL C).
 */
#if __IBMC__ || !__clang__ && __GNUC__ &&                                      \
		    (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 9))
#warning not using C11 stdatomic on legacy system
#define _Atomic volatile
#define atomic_load(x) (*(x))
#define atomic_store(x, y) (*(x) = (y))
#define ATOMIC_VAR_INIT(x) (x)
#else
#include <stdatomic.h>
#endif

#include "fmt.h"
#include "fmtdef.h"

enum { Maxfmt = 128 };

typedef struct Convfmt Convfmt;

struct Convfmt {
	int  c;
	Fmts fmt;
};

static struct {
	/*
	 * lock updates to fmt by calling __fmtlock, __fmtunlock.
	 * reads can start at nfmt and work backward without
	 * further locking. later fmtinstalls take priority over earlier
	 * ones because of the backwards loop.
	 * once installed, a format is never overwritten.
	 */
	_Atomic int nfmt;
	Convfmt     fmt[Maxfmt];
} fmtalloc = {
    /* clang-format off */
	ATOMIC_VAR_INIT(27),
	{
		{' ', __flagfmt},  {'#', __flagfmt},  {'%', __percentfmt},
		{'\'', __flagfmt}, {'+', __flagfmt},  {',', __flagfmt},
		{'-', __flagfmt},  {'C', __runefmt}, /* Plan 9 addition */
		{'E', __efgfmt},
		{'G', __efgfmt},
		{'S', __runesfmt},                /* Plan 9 addition */
		{'X', __ifmt},     {'b', __ifmt}, /* Plan 9 addition */
		{'c', __charfmt},  {'d', __ifmt},     {'e', __efgfmt},
		{'f', __efgfmt},   {'g', __efgfmt},   {'h', __flagfmt},
		{'l', __flagfmt},  {'n', __countfmt}, {'o', __ifmt},
		{'p', __ifmt},     {'r', __errfmt},   {'s', __strfmt},
		{'u', __flagfmt},
		{'x', __ifmt},
	}
    /* clang-format on */
};

int (*fmtdoquote)(int);

/*
 * __fmtlock() must be set
 */
static int __fmtinstall(int c, Fmts f) {
	Convfmt *p;
	int      i;

	if (c <= 0 || c >= 65536) {
		return -1;
	}
	if (!f) {
		f = __badfmt;
	}

	i = atomic_load(&fmtalloc.nfmt);
	if (i == Maxfmt) {
		return -1;
	}
	p = &fmtalloc.fmt[i];
	p->c = c;
	p->fmt = f;
	atomic_store(&fmtalloc.nfmt, i + 1);

	return 0;
}

int fmtinstall(int c, int (*f)(Fmt *)) {
	int ret;

	__fmtlock();
	ret = __fmtinstall(c, f);
	__fmtunlock();
	return ret;
}

static Fmts fmtfmt(int c) {
	Convfmt *p, *ep;

	ep = &fmtalloc.fmt[atomic_load(&fmtalloc.nfmt)];
	for (p = ep; p-- > fmtalloc.fmt;) {
		if (p->c == c) {
			return p->fmt;
		}
	}

	return __badfmt;
}

void *__fmtdispatch(Fmt *f, void *fmt, int isrunes) {
	Rune rune, r;
	int  i, n;

	f->flags = 0;
	f->width = f->prec = 0;

	for (;;) {
		if (isrunes) {
			r = *(Rune *)fmt;
			fmt = (Rune *)fmt + 1;
		} else {
			fmt = (char *)fmt + chartorune(&rune, (char *)fmt);
			r = rune;
		}
		f->r = r;
		switch (r) {
		case '\0':
			return NULL;
		case '.':
			f->flags |= FmtWidth | FmtPrec;
			continue;
		case '0':
			if (!(f->flags & FmtWidth)) {
				f->flags |= FmtZero;
				continue;
			}
			/* fall through */
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			i = 0;
			while (r >= '0' && r <= '9') {
				i = i * 10 + r - '0';
				if (isrunes) {
					r = *(Rune *)fmt;
					fmt = (Rune *)fmt + 1;
				} else {
					r = *(char *)fmt;
					fmt = (char *)fmt + 1;
				}
			}
			if (isrunes) {
				fmt = (Rune *)fmt - 1;
			} else {
				fmt = (char *)fmt - 1;
			}
		numflag:
			if (f->flags & FmtWidth) {
				f->flags |= FmtPrec;
				f->prec = i;
			} else {
				f->flags |= FmtWidth;
				f->width = i;
			}
			continue;
		case '*':
			i = va_arg(f->args, int);
			if (i < 0) {
				/*
				 * negative precision =>
				 * ignore the precision.
				 */
				if (f->flags & FmtPrec) {
					f->flags &= ~FmtPrec;
					f->prec = 0;
					continue;
				}
				i = -i;
				f->flags |= FmtLeft;
			}
			goto numflag;
		}
		n = (*fmtfmt(r))(f);
		if (n < 0) {
			return NULL;
		}
		if (n == 0) {
			return fmt;
		}
	}
}
