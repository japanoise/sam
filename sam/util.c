#include "sam.h"
#include <stdint.h>

char *getuser() {
	return getenv("USER")      ? getenv("USER")
	       : getenv("LOGNAME") ? getenv("LOGNAME")
				   : "nemo";
	/* Can't find the user, he must be nemo :^) */
}

void cvttorunes(char *p, int n, Rune *r, int *nb, int *nr, bool *nulls) {
	uint8_t *q;
	Rune    *s;
	int      j, w;

	/*
	 * Always guaranteed that n bytes may be interpreted
	 * without worrying about partial runes.  This may mean
	 * reading up to UTFmax-1 more bytes than n; the caller
	 * knows this.  If n is a firm limit, the caller should
	 * set p[n] = 0.
	 */
	q = (uint8_t *)p;
	s = r;
	for (j = 0; j < n; j += w) {
		if (*q < Runeself) {
			w = 1;
			*s = *q++;
		} else {
			w = chartorune(s, (char *)q);
			q += w;
		}
		if (*s) {
			s++;
		} else if (nulls) {
			*nulls = true;
		}
	}
	*nb = (char *)q - p;
	*nr = s - r;
}

void *fbufalloc(void) { return emalloc(BUFSIZE); }

void fbuffree(void *f) { free(f); }

uint min(uint a, uint b) {
	if (a < b) {
		return a;
	}
	return b;
}
