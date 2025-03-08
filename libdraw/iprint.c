#include <u.h>
#include <draw.h>
#include <stdarg.h>
#include <fmt.h>

int iprint(char *fmt, ...) {
	va_list arg;

	va_start(arg, fmt);
	vfprint(1, fmt, arg);
	va_end(arg);
	return 0;
}
