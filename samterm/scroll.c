/* Copyright (c) 1998 Lucent Technologies - All rights reserved. */
#include <u.h>
#include <libg.h>
#include <frame.h>
#include "flayer.h"
#include "samterm.h"

extern Bitmap *darkgrey;
extern Mouse   mouse;

Rectangle scrpos(Rectangle r, int64_t p0, int64_t p1, int64_t tot) {
	int64_t   h;
	Rectangle q;

	q = inset(r, 1);
	h = q.max.y - q.min.y;
	if (tot == 0) {
		return q;
	}
	if (tot > 1024L * 1024L) {
		tot >>= 10, p0 >>= 10, p1 >>= 10;
	}
	if (p0 > 0) {
		q.min.y += h * p0 / tot;
	}
	if (p1 < tot) {
		q.max.y -= h * (tot - p1) / tot;
	}
	if (q.max.y < q.min.y + 2) {
		if (q.min.y + 2 <= r.max.y) {
			q.max.y = q.min.y + 2;
		} else {
			q.min.y = q.max.y - 2;
		}
	}
	return q;
}

void scrflip(Flayer *l, Rectangle r) {
	if (rectclip(&r, l->scroll)) {
		bitblt2(l->f.b, r.min, l->f.b, r, F & ~D, 0, l->bg);
	}
}

void scrdraw(Flayer *l, int64_t tot) {
	Rectangle      r, r1, r2;
	Bitmap        *b;
	static Bitmap *x;
	int            h;

	if (l->f.b == 0) {
		panic("scrdraw");
	}
	r = l->scroll;
	r.min.x += 1; /* border between margin and bar */
	r1 = r;
	if (l->visible == All) {
		if (x == 0) {
			if (screensize(0, &h) == 0) {
				h = 2048;
			}
			x = balloc(Rect(0, 0, 32, h), l->f.b->ldepth);
			if (x == 0) {
				panic("scrdraw balloc");
			}
		}
		b = x;
		r1.min.x = 0;
		r1.max.x = Dx(r);
	} else {
		b = l->f.b;
	}
	bitblt2(b, r1.min, b, r1, F, 0, l->bg);
	texture(b, inset(r1, 1), darkgrey, S);
	r2 = scrpos(r1, l->origin, l->origin + l->f.nchars, tot);
	bitblt2(b, r2.min, b, r2, 0, 0, l->bg);
	if (b != l->f.b) {
		bitblt2(l->f.b, r.min, b, r1, S, 0, l->bg);
	}
}

/*
 * `pbut` appears to == `but` in all existing calls
 *
 * 1 = Left
 * 2 = Middle
 * 3 = Right
 */
void scroll(Flayer *l, int pbut, int but) {
	int       in = 0, oin;
	int64_t   tot = scrtotal(l);
	Rectangle scr, r, s, rt;
	int       x, y, my, oy, h;
	int64_t   p0; /* Goal point */

	s = inset(l->scroll, 1);
	x = s.min.x + FLSCROLLWID / 2;
	scr = scrpos(l->scroll, l->origin, l->origin + l->f.nchars, tot);
	r = scr;
	y = scr.min.y;
	my = mouse.xy.y;

	/* While the mouse button is down, calculate the scrollbar
	   position & draw it */
	do {
		oin = in;
		in = abs(x - mouse.xy.x) <= FLSCROLLWID / 2;
		if (oin != in) {
			scrflip(l, r);
		}
		if (in) {
			oy = y;
			my = mouse.xy.y;
			if (my < s.min.y) {
				my = s.min.y;
			}
			if (my >= s.max.y) {
				my = s.max.y;
			}
			if (!eqpt(mouse.xy, Pt(x, my))) {
				cursorset(Pt(x, my));
			}
			if (but == 1) {
				p0 = l->origin -
				     frcharofpt(&l->f, Pt(s.max.x, my));
				rt = scrpos(l->scroll, p0, p0 + l->f.nchars,
					    tot);
				y = rt.min.y;
			} else if (but == 2) {
				y = my;
				if (y > s.max.y - 2) {
					y = s.max.y - 2;
				}
			} else if (but == 3) {
				p0 = l->origin +
				     frcharofpt(&l->f, Pt(s.max.x, my));
				rt = scrpos(l->scroll, p0, p0 + l->f.nchars,
					    tot);
				y = rt.min.y;
			}
			if (y != oy) {
				scrflip(l, r);
				r = raddp(scr, Pt(0, y - scr.min.y));
				scrflip(l, r);
			}
		}
	} while (button(pbut));

	if (in) {
		h = s.max.y - s.min.y;
		scrflip(l, r);
		p0 = 0;
		if (but == 1) {
			/* LMB: scroll up */
			p0 = (int64_t)(my - s.min.y) / l->f.fheight + 1;
		} else if (but == 2) {
			/* MMB: jump */
			if (tot > 1024L * 1024L) {
				p0 = ((tot >> 10) * (y - s.min.y) / h) << 10;
			} else {
				p0 = tot * (y - s.min.y) / h;
			}
		} else if (but == 3) {
			/* RMB: scroll down */
			p0 = l->origin + frcharofpt(&l->f, Pt(s.max.x, my));
			while (p0 >= tot) {
				my--;
				p0 = l->origin +
				     frcharofpt(&l->f, Pt(s.max.x, my));
				if (p0 == 0) {
					break;
				}
			}
		}
		scrorigin(l, but, p0);
	}
}
