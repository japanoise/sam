#include <u.h>
#include <stdarg.h>
#include <fmt.h>

void (*_sysfatal)(char *, ...) = NULL;

void sysfatal(char *fmt, ...) {
	char    buf[256];
	va_list arg;

	va_start(arg, fmt);
	if (_sysfatal) {
		(*_sysfatal)(fmt, arg);
	}
	vseprint(buf, buf + sizeof buf, fmt, arg);
	va_end(arg);

	fprint(2, "%s: fatal: %s\n", "<prog>", buf);
	exit(1);
}
