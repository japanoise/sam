#include <u.h>
#include <draw.h>

Image *allocimagemix(Display *d, u32int color1, u32int color3) {
	Image        *t, *b;
	static Image *qmask;

	if (qmask == NULL) {
		qmask = allocimage(d, Rect(0, 0, 1, 1), GREY8, 1, 0x3F3F3FFF);
	}

	if (d->screenimage->depth <= 8) { /* create a 2×2 texture */
		t = allocimage(d, Rect(0, 0, 1, 1), d->screenimage->chan, 0,
			       color1);
		if (t == NULL) {
			return NULL;
		}

		b = allocimage(d, Rect(0, 0, 2, 2), d->screenimage->chan, 1,
			       color3);
		if (b == NULL) {
			freeimage(t);
			return NULL;
		}

		draw(b, Rect(0, 0, 1, 1), t, NULL, ZP);
		freeimage(t);
		return b;
	} else { /* use a solid color, blended using alpha */
		t = allocimage(d, Rect(0, 0, 1, 1), d->screenimage->chan, 1,
			       color1);
		if (t == NULL) {
			return NULL;
		}

		b = allocimage(d, Rect(0, 0, 1, 1), d->screenimage->chan, 1,
			       color3);
		if (b == NULL) {
			freeimage(t);
			return NULL;
		}

		draw(b, b->r, t, qmask, ZP);
		freeimage(t);
		return b;
	}
}
