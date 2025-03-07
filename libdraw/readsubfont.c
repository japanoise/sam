#include <u.h>
#include <draw.h>

Subfont *readsubfonti(Display *d, char *name, int fd, Image *ai, int dolock) {
	char      hdr[3 * 12 + 4 + 1];
	int       n;
	uchar    *p;
	Fontchar *fc;
	Subfont  *f;
	Image    *i;

	i = ai;
	if (i == NULL) {
		i = readimage(d, fd, dolock);
		if (i == NULL) {
			return NULL;
		}
	}
	if (read(fd, hdr, 3 * 12) != 3 * 12) {
		if (ai == NULL) {
			freeimage(i);
		}
		werrstr("rdsubfonfile: header read error: %r");
		return NULL;
	}
	n = atoi(hdr);
	p = malloc(6 * (n + 1));
	if (p == NULL) {
		goto Err;
	}
	if (read(fd, p, 6 * (n + 1)) != 6 * (n + 1)) {
		werrstr("rdsubfonfile: fontchar read error: %r");
	Err:
		if (ai == NULL) {
			freeimage(i);
		}
		free(p);
		return NULL;
	}
	fc = malloc(sizeof(Fontchar) * (n + 1));
	if (fc == NULL) {
		goto Err;
	}
	_unpackinfo(fc, p, n);
	if (dolock) {
		lockdisplay(d);
	}
	f = allocsubfont(name, n, atoi(hdr + 12), atoi(hdr + 24), fc, i);
	if (dolock) {
		unlockdisplay(d);
	}
	if (f == NULL) {
		free(fc);
		goto Err;
	}
	free(p);
	return f;
}

Subfont *readsubfont(Display *d, char *name, int fd, int dolock) {
	return readsubfonti(d, name, fd, NULL, dolock);
}

void _unpackinfo(Fontchar *fc, uchar *p, int n) {
	int j;

	for (j = 0; j <= n; j++) {
		fc->x = p[0] | (p[1] << 8);
		fc->top = p[2];
		fc->bottom = p[3];
		fc->left = p[4];
		fc->width = p[5];
		fc++;
		p += 6;
	}
}
