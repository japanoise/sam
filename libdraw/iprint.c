#include <u.h>
#include <libc.h>
#include <draw.h>
#include <stdarg.h>

int iprint(char *fmt, ...) {
	va_list arg;

	va_start(arg, fmt);
	vfprintf(stdout, fmt, arg);
	va_end(arg);
	return 0;
}
