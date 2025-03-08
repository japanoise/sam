#include <u.h>
#include <draw.h>
#include <memdraw.h>
#include <memlayer.h>

Memimage *memlalloc(Memscreen *s, Rectangle screenr, Refreshfn refreshfn,
		    void *refreshptr, u32int val) {
	Memlayer        *l;
	Memimage        *n;
	static Memimage *paint;

	if (paint == NULL) {
		paint = allocmemimage(Rect(0, 0, 1, 1), RGBA32);
		if (paint == NULL) {
			return NULL;
		}
		paint->flags |= Frepl;
		paint->clipr =
		    Rect(-0x3FFFFFF, -0x3FFFFFF, 0x3FFFFFF, 0x3FFFFFF);
	}

	n = allocmemimaged(screenr, s->image->chan, s->image->data, NULL);
	if (n == NULL) {
		return NULL;
	}
	l = malloc(sizeof(Memlayer));
	if (l == NULL) {
		free(n);
		return NULL;
	}

	l->screen = s;
	if (refreshfn) {
		l->save = NULL;
	} else {
		l->save = allocmemimage(screenr, s->image->chan);
		if (l->save == NULL) {
			free(l);
			free(n);
			return NULL;
		}
		/* allocmemimage doesn't initialize memory; this paints save
		 * area */
		if (val != DNofill) {
			memfillcolor(l->save, val);
		}
	}
	l->refreshfn = refreshfn;
	l->refreshptr = NULL; /* don't set it until we're done */
	l->screenr = screenr;
	l->delta = Pt(0, 0);

	n->data->ref++;
	n->zero = s->image->zero;
	n->width = s->image->width;
	n->layer = l;

	/* start with new window behind all existing ones */
	l->front = s->rearmost;
	l->rear = NULL;
	if (s->rearmost) {
		s->rearmost->layer->rear = n;
	}
	s->rearmost = n;
	if (s->frontmost == NULL) {
		s->frontmost = n;
	}
	l->clear = 0;

	/* now pull new window to front */
	_memltofrontfill(n, val != DNofill);
	l->refreshptr = refreshptr;

	/*
	 * paint with requested color; previously exposed areas are already
	 * right if this window has backing store, but just painting the whole
	 * thing is simplest.
	 */
	if (val != DNofill) {
		memsetchan(paint, n->chan);
		memfillcolor(paint, val);
		memdraw(n, n->r, paint, n->r.min, NULL, n->r.min, S);
	}
	return n;
}
