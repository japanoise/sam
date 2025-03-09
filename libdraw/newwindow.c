#include <u.h>
#include <draw.h>
#include <bio.h>
#include <sys/mount.h>

/* Connect us to new window, if possible */
int newwindow(char *str) {
	int   fd;
	char *wsys;
	char  buf[256];

	wsys = getenv("wsys");
	if (wsys == NULL) {
		return -1;
	}
	fd = open(wsys, ORDWR);
	free(wsys);
	if (fd < 0) {
		return -1;
	}
	p9rfork(RFNAMEG);
	if (str) {
		snprintf(buf, sizeof buf, "new %s", str);
	} else {
		strcpy(buf, "new");
	}
	return mount(fd, -1, "/dev", MBEFORE, buf);
}
