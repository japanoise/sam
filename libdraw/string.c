#include <u.h>
#include <draw.h>

enum { Max = 100 };

Point string(Image *dst, Point pt, Image *src, Point sp, Font *f, char *s) {
	return _string(dst, pt, src, sp, f, s, NULL, 1 << 24, dst->clipr, NULL,
		       ZP, SoverD);
}

Point stringop(Image *dst, Point pt, Image *src, Point sp, Font *f, char *s,
	       Drawop op) {
	return _string(dst, pt, src, sp, f, s, NULL, 1 << 24, dst->clipr, NULL,
		       ZP, op);
}

Point stringn(Image *dst, Point pt, Image *src, Point sp, Font *f, char *s,
	      int len) {
	return _string(dst, pt, src, sp, f, s, NULL, len, dst->clipr, NULL, ZP,
		       SoverD);
}

Point stringnop(Image *dst, Point pt, Image *src, Point sp, Font *f, char *s,
		int len, Drawop op) {
	return _string(dst, pt, src, sp, f, s, NULL, len, dst->clipr, NULL, ZP,
		       op);
}

Point runestring(Image *dst, Point pt, Image *src, Point sp, Font *f, Rune *r) {
	return _string(dst, pt, src, sp, f, NULL, r, 1 << 24, dst->clipr, NULL,
		       ZP, SoverD);
}

Point runestringop(Image *dst, Point pt, Image *src, Point sp, Font *f, Rune *r,
		   Drawop op) {
	return _string(dst, pt, src, sp, f, NULL, r, 1 << 24, dst->clipr, NULL,
		       ZP, op);
}

Point runestringn(Image *dst, Point pt, Image *src, Point sp, Font *f, Rune *r,
		  int len) {
	return _string(dst, pt, src, sp, f, NULL, r, len, dst->clipr, NULL, ZP,
		       SoverD);
}

Point runestringnop(Image *dst, Point pt, Image *src, Point sp, Font *f,
		    Rune *r, int len, Drawop op) {
	return _string(dst, pt, src, sp, f, NULL, r, len, dst->clipr, NULL, ZP,
		       op);
}

Point _string(Image *dst, Point pt, Image *src, Point sp, Font *f, char *s,
	      Rune *r, int len, Rectangle clipr, Image *bg, Point bgp,
	      Drawop op) {
	int      m, n, wid, max;
	ushort   cbuf[Max], *c, *ec;
	uchar   *b;
	char    *subfontname;
	char   **sptr;
	Rune   **rptr;
	Font    *def;
	Subfont *sf;

	if (len < 0) {
		fprint(2, "libdraw: _string len=%d", len);
		abort();
	}

	if (s == NULL) {
		s = "";
		sptr = NULL;
	} else {
		sptr = &s;
	}
	if (r == NULL) {
		r = (Rune *)L"";
		rptr = NULL;
	} else {
		rptr = &r;
	}
	sf = NULL;
#if defined(__AIX__)
	while ((*s || *rptr) && len) {
#else
	while ((*s || *r) && len) {
#endif
		max = Max;
		if (len < max) {
			max = len;
		}
		n = cachechars(f, sptr, rptr, cbuf, max, &wid, &subfontname);
		if (n > 0) {
			_setdrawop(dst->display, op);

			m = 47 + 2 * n;
			if (bg) {
				m += 4 + 2 * 4;
			}
			b = bufimage(dst->display, m);
			if (b == 0) {
				fprint(2, "string: %r\n");
				break;
			}
			if (bg) {
				b[0] = 'x';
			} else {
				b[0] = 's';
			}
			BPLONG(b + 1, dst->id);
			BPLONG(b + 5, src->id);
			BPLONG(b + 9, f->cacheimage->id);
			BPLONG(b + 13, pt.x);
			BPLONG(b + 17, pt.y + f->ascent);
			BPLONG(b + 21, clipr.min.x);
			BPLONG(b + 25, clipr.min.y);
			BPLONG(b + 29, clipr.max.x);
			BPLONG(b + 33, clipr.max.y);
			BPLONG(b + 37, sp.x);
			BPLONG(b + 41, sp.y);
			BPSHORT(b + 45, n);
			b += 47;
			if (bg) {
				BPLONG(b, bg->id);
				BPLONG(b + 4, bgp.x);
				BPLONG(b + 8, bgp.y);
				b += 12;
			}
			ec = &cbuf[n];
			for (c = cbuf; c < ec; c++, b += 2) {
				BPSHORT(b, *c);
			}
			pt.x += wid;
			bgp.x += wid;
			agefont(f);
			len -= n;
		}
		if (subfontname) {
			freesubfont(sf);
			if ((sf = _getsubfont(f->display, subfontname)) == 0) {
				def =
				    f->display ? f->display->defaultfont : NULL;
				if (def && f != def) {
					f = def;
				} else {
					break;
				}
			}
			/*
			 * must not free sf until cachechars has found it in the
			 * cache and picked up its own reference.
			 */
		}
	}
	return pt;
}
