#include <u.h>
#include <draw.h>

static Rune empty[] = {0};

int _stringnwidth(Font *f, char *s, Rune *r, int len) {
	int   wid, twid, n, max, l;
	char *name;

	enum { Max = 64 };

	ushort   cbuf[Max];
	Rune     rune, **rptr;
	char    *subfontname, **sptr;
	Font    *def;
	Subfont *sf;

	if (s == NULL) {
		s = "";
		sptr = NULL;
	} else {
		sptr = &s;
	}
	if (r == NULL) {
		r = empty;
		rptr = NULL;
	} else {
		rptr = &r;
	}
	twid = 0;
	while (len > 0 && (*s || *r)) {
		max = Max;
		if (len < max) {
			max = len;
		}
		n = 0;
		sf = NULL;
		while ((l = cachechars(f, sptr, rptr, cbuf, max, &wid,
				       &subfontname)) <= 0) {
			if (++n > 10) {
				if (*r) {
					rune = *r;
				} else {
					chartorune(&rune, s);
				}
				if (f->name != NULL) {
					name = f->name;
				} else {
					name = "unnamed font";
				}
				freesubfont(sf);
				fprintf(stderr,
					"stringwidth: bad character set for "
					"rune 0x%.4ux in %s\n",
					rune, name);
				return twid;
			}
			if (subfontname) {
				freesubfont(sf);
				if ((sf = _getsubfont(f->display,
						      subfontname)) == 0) {
					def = f->display
						  ? f->display->defaultfont
						  : NULL;
					if (def && f != def) {
						f = def;
					} else {
						break;
					}
				}
				/*
				 * must not free sf until cachechars has found
				 * it in the cache and picked up its own
				 * reference.
				 */
			}
		}
		freesubfont(sf);
		agefont(f);
		twid += wid;
		len -= l;
	}
	return twid;
}

int stringnwidth(Font *f, char *s, int len) {
	return _stringnwidth(f, s, NULL, len);
}

int stringwidth(Font *f, char *s) { return _stringnwidth(f, s, NULL, 1 << 24); }

Point stringsize(Font *f, char *s) {
	return Pt(_stringnwidth(f, s, NULL, 1 << 24), f->height);
}

int runestringnwidth(Font *f, Rune *r, int len) {
	return _stringnwidth(f, NULL, r, len);
}

int runestringwidth(Font *f, Rune *r) {
	return _stringnwidth(f, NULL, r, 1 << 24);
}

Point runestringsize(Font *f, Rune *r) {
	return Pt(_stringnwidth(f, NULL, r, 1 << 24), f->height);
}
