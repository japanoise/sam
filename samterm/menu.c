/* Copyright (c) 1998 Lucent Technologies - All rights reserved. */
#include <u.h>
#include <libg.h>
#include <frame.h>
#include "flayer.h"
#include "samterm.h"

uint8_t **name = NULL; /* first byte is ' ' or '\'': modified state */
Text    **text = NULL; /* pointer to Text associated with file */
uint16_t *tag = NULL;  /* text[i].tag, even if text[i] not defined */
int       nname;
int       mw;

int menucap;

char *genmenu3(int);
char *genmenu2(int);
char *genmenu2c(int);

extern uint64_t _bgpixel;

enum Menu2 {
	Cut,
	Paste,
	Snarf,
	Look,
	Exch,
	Search,
	NMENU2 = Search,
	Send = Search,
	NMENU2C
};

enum Menu3 { New, Zerox, Reshape, Close, Write, NMENU3 };

char *menu2str[] = {
    "cut", "paste", "snarf", "look", "<exch>", 0, /* storage for last pattern */
};

char *menu3str[] = {
    "new", "zerox", "reshape", "close", "write",
};

Menu menu2 = {0, genmenu2};
Menu menu2c = {0, genmenu2c};
Menu menu3 = {0, genmenu3};

void menu2hit(void) {
	Text *t = (Text *)which->user1;
	int   w = which - t->l;
	int   m;

	m = menuhit(2, &mouse, t == &cmd ? &menu2c : &menu2);
	if (lock || t->lock) {
		return;
	}

	switch (m) {
	case Cut:
		cut(t, w, true, true);
		break;

	case Paste:
		paste(t, w);
		break;

	case Snarf:
		snarf(t, w);
		break;

	case Exch:
		snarf(t, w);
		outT0(Tstartsnarf);
		setlock();
		break;

	case Look:
		outTsll(Tlook, t->tag, which->p0, which->p1);
		setlock();
		break;

	case Search:
		outcmd();
		if (t == &cmd) {
			outTsll(Tsend, 0 /*ignored*/, which->p0, which->p1);
		} else {
			outT0(Tsearch);
		}
		setlock();
		break;
	}
}

void menu3hit(void) {
	Rectangle r;
	Flayer   *l;
	int       m, i;
	Text     *t;

	mw = -1;
	switch (m = menuhit(3, &mouse, &menu3)) {
	case -1:
		break;

	case New:
		if (!lock) {
			sweeptext(1, 0);
		}
		break;

	case Zerox:
	case Reshape:
		if (!lock) {
			cursorswitch(BullseyeCursor);
			buttons(Down);
			if ((mouse.buttons & 4) && (l = flwhich(mouse.xy)) &&
			    getr(&r)) {
				duplicate(l, r, l->f.font, m == Reshape);
			} else {
				cursorswitch(cursor);
			}
			buttons(Up);
		}
		break;

	case Close:
		if (!lock) {
			cursorswitch(BullseyeCursor);
			buttons(Down);
			if ((mouse.buttons & 4) && (l = flwhich(mouse.xy)) &&
			    !lock) {
				t = (Text *)l->user1;
				if (t->nwin > 1) {
					closeup(l);
				} else if (t != &cmd) {
					outTs(Tclose, t->tag);
					setlock();
				}
			}
			cursorswitch(cursor);
			buttons(Up);
		}
		break;

	case Write:
		if (!lock) {
			cursorswitch(BullseyeCursor);
			buttons(Down);
			if ((mouse.buttons & 4) && (l = flwhich(mouse.xy))) {
				outTs(Twrite, ((Text *)l->user1)->tag);
				setlock();
			} else {
				cursorswitch(cursor);
			}
			buttons(Up);
		}
		break;

	default:
		if ((t = text[m - NMENU3])) {
			i = t->front;
			if (t->nwin == 0 || t->l[i].textfn == 0) {
				return; /* not ready yet; try again later */
			}
			if (t->nwin > 1 && which == &t->l[i]) {
				do {
					if (++i == NL) {
						i = 0;
					}
				} while (i != t->front && t->l[i].textfn == 0);
			}
			current(&t->l[i]);
			if (followfocus) {
				flupfront(&t->l[i]);
			}
		} else if (!lock) {
			sweeptext(0, tag[m - NMENU3]);
		}
		break;
	}
}

Text *sweeptext(int new, int tag) {
	Rectangle r;
	Text     *t;

	if (getr(&r) && (t = malloc(sizeof(Text)))) {
		memset((void *)t, 0, sizeof(Text));
		current((Flayer *)0);
		flnew(&t->l[0], stgettext, 0, (char *)t);
		flinit(&t->l[0], r, font, getbg()); /*bnl*/
		t->nwin = 1;
		rinit(&t->rasp);
		if (new) {
			startnewfile(Tstartnewfile, t);
		} else {
			rinit(&t->rasp);
			t->tag = tag;
			startfile(t);
		}
		return t;
	}
	return 0;
}

int whichmenu(int tg) {
	int i;

	for (i = 0; i < nname; i++) {
		if (tag[i] == tg) {
			return i;
		}
	}
	return -1;
}

void menufree(void) {
	free(name);
	free(text);
	free(tag);
}

void menuins(int n, uint8_t *s, Text *t, int m, int tg) {
	int i;

	if (name == NULL) {
		menucap = 16;
		name = malloc(sizeof(uint8_t *) * menucap);
		text = malloc(sizeof(Text *) * menucap);
		tag = malloc(sizeof(uint16_t) * menucap);
		atexit(menufree);
	} else if (nname >= menucap - 2) {
		menucap <<= 1;
		name = realloc(name, sizeof(uint8_t *) * menucap);
		text = realloc(text, sizeof(Text *) * menucap);
		tag = realloc(tag, sizeof(uint16_t) * menucap);
	}

	for (i = nname; i > n; --i) {
		name[i] = name[i - 1], text[i] = text[i - 1],
		tag[i] = tag[i - 1];
	}
	text[n] = t;
	tag[n] = tg;
	name[n] = alloc(strlen((char *)s) + 2);
	name[n][0] = m;
	strcpy((char *)name[n] + 1, (char *)s);
	nname++;
	menu3.lasthit = n + NMENU3;
}

void menudel(int n) {
	int i;

	if (nname == 0 || n >= nname || text[n]) {
		panic("menudel");
	}
	free(name[n]);
	--nname;
	for (i = n; i < nname; i++) {
		name[i] = name[i + 1], text[i] = text[i + 1],
		tag[i] = tag[i + 1];
	}
}

void setpat(char *s) {
	static char pat[17];

	pat[0] = '/';
	strncpy(pat + 1, s, 15);
	menu2str[Search] = pat;
}

bool haspat(void) { return (bool)(menu2str[Search]); }

#define NBUF 64
static uint8_t buf[NBUF * UTFmax] = {' ', ' ', ' ', ' '};

char *paren(char *s) {
	uint8_t *t = buf;

	*t++ = '(';
	do
		;
	while ((*t++ = *s++));
	t[-1] = ')';
	*t = 0;
	return (char *)buf;
}

char *genmenu2(int n) {
	Text *t = (Text *)which->user1;
	char *p;
	if (n >= NMENU2 + (menu2str[Search] != 0)) {
		return 0;
	}
	p = menu2str[n];
	if ((!lock && !t->lock) || n == Search || n == Look) {
		return p;
	}
	return paren(p);
}

char *genmenu2c(int n) {
	Text *t = (Text *)which->user1;
	char *p;
	if (n >= NMENU2C) {
		return 0;
	}
	if (n == Send) {
		p = "send";
	} else {
		p = menu2str[n];
	}
	if (!lock && !t->lock) {
		return p;
	}
	return paren(p);
}

char *genmenu3(int n) {
	Text *t;
	int   c, i, k, l, w;
	Rune  r;
	char *p;

	if (n >= NMENU3 + nname) {
		return 0;
	}
	if (n < NMENU3) {
		p = menu3str[n];
		if (lock) {
			p = paren(p);
		}
		return p;
	}
	n -= NMENU3;
	if (n == 0) { /* unless we've been fooled, this is cmd */
		return (char *)&name[n][1];
	}
	if (mw == -1) {
		mw = 7; /* strlen("~~sam~~"); */
		for (i = 1; i < nname; i++) {
			w = utflen((char *)name[i] + 1) +
			    4; /* include "'+. " */
			if (w > mw) {
				mw = w;
			}
		}
	}
	if (mw > NBUF) {
		mw = NBUF;
	}
	t = text[n];
	buf[0] = name[n][0];
	buf[1] = '-';
	buf[2] = ' ';
	buf[3] = ' ';
	if (t) {
		if (t->nwin == 1) {
			buf[1] = '+';
		} else if (t->nwin > 1) {
			buf[1] = '*';
		}
		if (work && t == (Text *)work->user1) {
			buf[2] = '.';
			if (modified) {
				buf[0] = '\'';
			}
		}
	}
	l = utflen((char *)name[n] + 1);
	if (l > NBUF - 4 - 2) {
		i = 4;
		k = 1;
		while (i < NBUF / 2) {
			k += chartorune(&r, (char *)name[n] + k);
			i++;
		}
		c = name[n][k];
		name[n][k] = 0;
		strcpy((char *)buf + 4, (char *)name[n] + 1);
		name[n][k] = c;
		strcat((char *)buf, "...");
		while ((l - i) >= NBUF / 2 - 4) {
			k += chartorune(&r, (char *)name[n] + k);
			i++;
		}
		strcat((char *)buf, (char *)name[n] + k);
	} else {
		strcpy((char *)buf + 4, (char *)name[n] + 1);
	}
	i = utflen((char *)buf);
	k = strlen((char *)buf);
	while (i < mw && k < sizeof buf - 1) {
		buf[k++] = ' ';
		i++;
	}
	buf[k] = 0;
	return (char *)buf;
}
