#include <u.h>
#include <draw.h>
#include <bio.h>

/*
 * I don't want too many of these,
 * but the ones we have are just too useful.
 */
static struct {
	char *old;
	char *new;
} replace[] = {
    "#9",
    NULL, /* must be first */
    "#d",
    "/dev/fd",
};

char *get9root(void) {
	static char *s;

	if (s) {
		return s;
	}

	if ((s = getenv("PLAN9")) != 0) {
		return s;
	}
	/* bweh */
	s = "/tmp";
	return s;
}

char *unsharp(char *old) {
	char *new;
	int i, olen, nlen, len;

	if (replace[0].new == NULL) {
		replace[0].new = get9root();
	}

	for (i = 0; i < nelem(replace); i++) {
		if (!replace[i].new) {
			continue;
		}
		olen = strlen(replace[i].old);
		if (strncmp(old, replace[i].old, olen) != 0 ||
		    (old[olen] != '\0' && old[olen] != '/')) {
			continue;
		}
		nlen = strlen(replace[i].new);
		len = strlen(old) + nlen - olen;
		new = malloc(len + 1);
		if (new == NULL) {
			/* Most callers won't check the return value... */
			fprint(2, "out of memory translating %s to %s%s", old,
			       replace[i].new, old + olen);
			abort();
		}
		strcpy(new, replace[i].new);
		strcpy(new + nlen, old + olen);
		assert(strlen(new) == len);
		return new;
	}
	return old;
}

extern vlong _drawflength(int);
int          _fontpipe(char *);

int parsefontscale(char *name, char **base) {
	char *p;
	int   scale;

	p = name;
	scale = 0;
	while ('0' <= *p && *p <= '9') {
		scale = scale * 10 + *p - '0';
		p++;
	}
	if (*p == '*' && scale > 0) {
		*base = p + 1;
	} else {
		*base = name;
		scale = 1;
	}
	return scale;
}

extern char _defontfile[];

Font *openfont1(Display *d, char *name) {
	Font *fnt;
	int   fd, i, n, scale;
	char *buf, *nambuf, *nambuf0, *fname, *freename;

	nambuf = 0;
	freename = NULL;
	scale = parsefontscale(name, &fname);

	if (strcmp(fname, "*default*") == 0) {
		buf = strdup(_defontfile);
		goto build;
	}
	fd = open(fname, OREAD);
	if (fd < 0 && strncmp(fname, "/lib/font/bit/", 14) == 0) {
		nambuf = smprint("#9/font/%s", fname + 14);
		if (nambuf == NULL) {
			return 0;
		}
		nambuf0 = unsharp(nambuf);
		if (nambuf0 != nambuf) {
			free(nambuf);
		}
		nambuf = nambuf0;
		if (nambuf == NULL) {
			return 0;
		}
		if ((fd = open(nambuf, OREAD)) < 0) {
			free(nambuf);
			return 0;
		}
		if (scale > 1) {
			name = smprint("%d*%s", scale, nambuf);
			freename = name;
		} else {
			name = nambuf;
		}
	}
	if (fd >= 0) {
		n = _drawflength(fd);
	}
	if (fd < 0 && strncmp(fname, "/mnt/font/", 10) == 0) {
		fd = _fontpipe(fname + 10);
		n = 1024 * 1024;
	}
	if (fd < 0) {
		free(nambuf);
		free(freename);
		return 0;
	}

	buf = malloc(n + 1);
	if (buf == 0) {
		close(fd);
		free(nambuf);
		free(freename);
		return 0;
	}
	i = readn(fd, buf, n);
	close(fd);
	if (i <= 0) {
		free(buf);
		free(nambuf);
		free(freename);
		return 0;
	}
	buf[i] = 0;
build:
	fnt = buildfont(d, buf, name);
	free(buf);
	free(nambuf);
	free(freename);
	if (scale != 1) {
		fnt->scale = scale;
		fnt->height *= scale;
		fnt->ascent *= scale;
		fnt->width *= scale;
	}
	return fnt;
}

void swapfont(Font *targ, Font **oldp, Font **newp) {
	Font f, *old, *new;

	if (targ != *oldp) {
		fprint(2, "bad swapfont %p %p %p", targ, *oldp, *newp);
		abort();
	}

	old = *oldp;
	new = *newp;

	f.name = old->name;
	f.display = old->display;
	f.height = old->height;
	f.ascent = old->ascent;
	f.width = old->width;
	f.nsub = old->nsub;
	f.age = old->age;
	f.maxdepth = old->maxdepth;
	f.ncache = old->ncache;
	f.nsubf = old->nsubf;
	f.scale = old->scale;
	f.cache = old->cache;
	f.subf = old->subf;
	f.sub = old->sub;
	f.cacheimage = old->cacheimage;

	old->name = new->name;
	old->display = new->display;
	old->height = new->height;
	old->ascent = new->ascent;
	old->width = new->width;
	old->nsub = new->nsub;
	old->age = new->age;
	old->maxdepth = new->maxdepth;
	old->ncache = new->ncache;
	old->nsubf = new->nsubf;
	old->scale = new->scale;
	old->cache = new->cache;
	old->subf = new->subf;
	old->sub = new->sub;
	old->cacheimage = new->cacheimage;

	new->name = f.name;
	new->display = f.display;
	new->height = f.height;
	new->ascent = f.ascent;
	new->width = f.width;
	new->nsub = f.nsub;
	new->age = f.age;
	new->maxdepth = f.maxdepth;
	new->ncache = f.ncache;
	new->nsubf = f.nsubf;
	new->scale = f.scale;
	new->cache = f.cache;
	new->subf = f.subf;
	new->sub = f.sub;
	new->cacheimage = f.cacheimage;

	*oldp = new;
	*newp = old;
}

static char *hidpiname(Font *f) {
	char *p, *q;
	int   size;

	// If font name has form x,y return y.
	p = strchr(f->namespec, ',');
	if (p != NULL) {
		return strdup(p + 1);
	}

	// If font name is /mnt/font/Name/Size/font, scale Size.
	if (strncmp(f->name, "/mnt/font/", 10) == 0) {
		p = strchr(f->name + 10, '/');
		if (p == NULL || *++p < '0' || *p > '9') {
			goto scale;
		}
		q = p;
		size = 0;
		while ('0' <= *q && *q <= '9') {
			size = size * 10 + *q++ - '0';
		}
		return smprint("%.*s%d%s", utfnlen(f->name, p - f->name),
			       f->name, size * 2, q);
	}

	// Otherwise use pixel doubling.
scale:
	return smprint("%d*%s", f->scale * 2, f->name);
}

void loadhidpi(Font *f) {
	char *name;
	Font *fnew;

	if (f->hidpi == f) {
		return;
	}
	if (f->hidpi != NULL) {
		swapfont(f, &f->lodpi, &f->hidpi);
		return;
	}

	name = hidpiname(f);
	fnew = openfont1(f->display, name);
	if (fnew == NULL) {
		return;
	}
	f->hidpi = fnew;
	free(name);

	swapfont(f, &f->lodpi, &f->hidpi);
}

Font *openfont(Display *d, char *name) {
	Font *f;
	char *p;
	char *namespec;

	// If font name has form x,y use x for lodpi, y for hidpi.
	name = strdup(name);
	namespec = strdup(name);
	if ((p = strchr(name, ',')) != NULL) {
		*p = '\0';
	}

	f = openfont1(d, name);
	if (!f) {
		return NULL;
	}
	f->lodpi = f;
	free(f->namespec);
	f->namespec = namespec;

	/* add to display list for when dpi changes */
	/* d can be NULL when invoked from mc. */
	if (d != NULL) {
		f->ondisplaylist = 1;
		f->prev = d->lastfont;
		f->next = NULL;
		if (f->prev) {
			f->prev->next = f;
		} else {
			d->firstfont = f;
		}
		d->lastfont = f;

		/* if this is a hi-dpi display, find hi-dpi version and swap */
		if (d->dpi >= DefaultDPI * 3 / 2) {
			loadhidpi(f);
		}
	}

	free(name);

	return f;
}

int _fontpipe(char *name) {
	int  p[2];
	char c;
	char buf[1024], *argv[10];
	int  nbuf, pid;

	if (pipe(p) < 0) {
		return -1;
	}
	pid = p9rfork(RFNOWAIT | RFFDG | RFPROC);
	if (pid < 0) {
		close(p[0]);
		close(p[1]);
		return -1;
	}
	if (pid == 0) {
		close(p[0]);
		p9dup(p[1], 1);
		p9dup(p[1], 2);
		if (p[1] > 2) {
			close(p[1]);
		}
		argv[0] = "fontsrv";
		argv[1] = "-pp";
		argv[2] = name;
		argv[3] = NULL;
		execvp("fontsrv", argv);
		print("exec fontsrv: %r\n");
		_exit(0);
	}
	close(p[1]);

	// success marked with leading \001.
	// otherwise an error happened.
	for (nbuf = 0; nbuf < sizeof buf - 1; nbuf++) {
		if (read(p[0], &c, 1) < 1 || c == '\n') {
			buf[nbuf] = '\0';
			werrstr(buf);
			close(p[0]);
			return -1;
		}
		if (c == '\001') {
			break;
		}
	}
	return p[0];
}
