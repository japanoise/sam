#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>
#include <memlayer.h>
#include <mouse.h>
#include <cursor.h>
#include <keyboard.h>
#include <drawfcall.h>
#include "devdraw.h"

enum { Nbutton = 10 };

static int debug;

static struct {
	int b[Nbutton];
	int init;
} map;

static void initmap(void) {
	char *p;
	int   i;

	p = getenv("mousedebug");
	if (p && p[0]) {
		debug = atoi(p);
	}

	for (i = 0; i < Nbutton; i++) {
		map.b[i] = i;
	}
	map.init = 1;
	p = getenv("mousebuttonmap");
	if (p) {
		for (i = 0; i < Nbutton && p[i]; i++) {
			if ('0' <= p[i] && p[i] <= '9') {
				map.b[i] = p[i] - '1';
			}
		}
	}
	if (debug) {
		fprintf(stderr, "mousemap: ");
		for (i = 0; i < Nbutton; i++) {
			fprintf(stderr, " %d", 1 + map.b[i]);
		}
		fprintf(stderr, "\n");
	}
}

int mouseswap(int but) {
	int i;
	int nbut;

	if (!map.init) {
		initmap();
	}

	nbut = 0;
	for (i = 0; i < Nbutton; i++) {
		if ((but & (1 << i)) && map.b[i] >= 0) {
			nbut |= 1 << map.b[i];
		}
	}
	if (debug) {
		fprintf(stderr, "swap %#b -> %#b\n", but, nbut);
	}
	return nbut;
}
