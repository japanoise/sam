/*
 * Parse /lib/keyboard to create latin1.h table for kernel.
 * mklatinkbd -r prints an array of integers rather than a Rune string literal.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>

int rflag;
int xflag;

enum {
	MAXLD = 2, /* latin1.c assumes this is 2 */
};

char *head = ""
	     "/*\n"
	     " * This is automatically generated by %s from /lib/keyboard\n"
	     " * Edit /lib/keyboard instead.\n"
	     " */\n";

/*
 * latin1.c assumes that strlen(ld) is at most 2.
 * It also assumes that latintab[i].ld can be a prefix of latintab[j].ld
 * only when j < i.  We ensure this by sorting the output by prefix length.
 * The so array is indexed by the character value.
 */

typedef struct Trie Trie;

struct Trie {
	int   n; /* of characters r */
	char  seq[MAXLD + 1 + 1];
	Rune  r[256];
	Trie *link[256];
};

Trie *root;

Trie *mktrie(char *seq) {
	uchar *q;
	Trie **tp;

	if (root == nil) {
		root = malloc(sizeof *root);
		memset(root, 0, sizeof *root);
	}

	assert(seq[0] != '\0');

	tp = &root;
	for (q = (uchar *)seq; *(q + 1) != '\0'; q++) {
		tp = &(*tp)->link[*q];
		if (*tp == nil) {
			*tp = malloc(sizeof(**tp));
			assert(*tp != nil);
			memset(*tp, 0, sizeof(**tp));
			strcpy((*tp)->seq, seq);
			(*tp)->seq[q + 1 - (uchar *)seq] = '\0';
		}
	}

	assert(*tp != nil);
	return *tp;
}

/* add character sequence s meaning rune r */
void insert(char *s, Rune r) {
	uchar lastc;
	int   len;
	Trie *t;

	len = strlen(s);
	lastc = (uchar)s[len - 1];

	t = mktrie(s);
	if (t->r[lastc]) {
		fprint(2, "warning: table duplicate: %s is %C and %C\n", s,
		       t->r[lastc], r);
		return;
	}
	t->r[lastc] = r;
	t->n++;
}

void cprintchar(Biobuf *b, int c) {
	/* print a byte c safe for a C string. */
	switch (c) {
	case '\'':
	case '\"':
	case '\\':
		Bprint(b, "\\%c", c);
		break;
	case '\t':
		Bprint(b, "\\t");
		break;
	default:
		if (isascii(c) && isprint(c)) {
			Bprint(b, "%c", c);
		} else {
			Bprint(b, "\\x%.2x", c);
		}
		break;
	}
}

void cprints(Biobuf *b, char *p) {
	while (*p != '\0') {
		cprintchar(b, *p++);
	}
}

void xprint(Biobuf *b, int c) {}

void printtrie(Biobuf *b, Trie *t) {
	int   i;
	char *p;

	for (i = 0; i < 256; i++) {
		if (t->link[i]) {
			printtrie(b, t->link[i]);
		}
	}
	if (t->n == 0) {
		return;
	}

	if (xflag) {
		for (i = 0; i < 256; i++) {
			if (t->r[i] == 0) {
				continue;
			}
			Bprint(b, "<Multi_key>");
			for (p = t->seq; *p; p++) {
				Bprint(b, " %k", *p);
			}
			Bprint(b, " %k : \"%C\" U%04X\n", i, t->r[i], t->r[i]);
		}
		return;
	}

	Bprint(b, "\t\"");
	cprints(b, t->seq);
	Bprint(b, "\", \"");
	for (i = 0; i < 256; i++) {
		if (t->r[i]) {
			cprintchar(b, i);
		}
	}
	Bprint(b, "\",\t");
	if (rflag) {
		Bprint(b, "{");
		for (i = 0; i < 256; i++) {
			if (t->r[i]) {
				Bprint(b, " 0x%.4ux,", t->r[i]);
			}
		}
		Bprint(b, " }");
	} else {
		Bprint(b, "L\"");
		for (i = 0; i < 256; i++) {
			if (t->r[i]) {
				Bprint(b, "%C", t->r[i]);
			}
		}
		Bprint(b, "\"");
	}
	Bprint(b, ",\n");
}

void readfile(char *fname) {
	Biobuf *b;
	char   *line, *p;
	char   *seq;
	int     inseq;
	int     lineno;
	Rune    r;

	if ((b = Bopen(fname, OREAD)) == 0) {
		fprint(2, "cannot open \"%s\": %r\n", fname);
		exits("open");
	}

	lineno = 0;
	while ((line = Brdline(b, '\n')) != 0) {
		lineno++;
		if (line[0] == '#') {
			continue;
		}

		r = strtol(line, nil, 16);
		p = strchr(line, ' ');
		if (r == 0 || (p != line + 4 && p != line + 5) || p[0] != ' ' ||
		    (p == line + 4 && p[1] != ' ')) {
			fprint(2, "%s:%d: cannot parse line\n", fname, lineno);
			continue;
		}

		p = line + 6;
		/*	00AE  Or rO       ®	registered trade mark sign
		 */
		for (inseq = 1, seq = p; (uchar)*p < Runeself; p++) {
			if (*p == '\0' || isspace(*p)) {
				if (inseq && p - seq >= 2) {
					*p = '\0';
					inseq = 0;
					insert(seq, r);
					*p = ' ';
				}
				if (*p == '\0') {
					break;
				}
			} else {
				if (!inseq) {
					seq = p;
					inseq = 1;
				}
			}
		}
	}
}

void usage(void) {
	fprint(2, "usage: mklatinkbd [-r] [/lib/keyboard]\n");
	exits("usage");
}

int kfmt(Fmt *);

void main(int argc, char **argv) {
	int    i;
	Biobuf bout;

	ARGBEGIN {
	case 'r': /* print rune values */
		rflag = 1;
		break;
	case 'x':
		xflag = 1;
		break;
	default:
		usage();
	}
	ARGEND

	if (argc > 1) {
		usage();
	}

	fmtinstall('k', kfmt);
	readfile(argc == 1 ? argv[0] : "/dev/stdin");

	Binit(&bout, 1, OWRITE);
	if (xflag) {
		Bprint(&bout, "# Generated by mklatinkbd -x; do not edit.\n");
		for (i = 0x20; i < 0x10000; i++) {
			Bprint(&bout,
			       "<Multi_key> <X> <%x> <%x> <%x> <%x> : \"%C\" "
			       "U%04X\n",
			       (i >> 12) & 0xf, (i >> 8) & 0xf, (i >> 4) & 0xf,
			       i & 0xf, i, i);
		}
	}
	if (root) {
		printtrie(&bout, root);
	}
	exits(0);
}

// X11 key names

struct {
	int   c;
	char *s;
} xkey[] = {' ',  "space",       '!',  "exclam",
	    '"',  "quotedbl",    '#',  "numbersign",
	    '$',  "dollar",      '%',  "percent",
	    '&',  "ampersand",   '\'', "apostrophe",
	    '(',  "parenleft",   ')',  "parenright",
	    '*',  "asterisk",    '+',  "plus",
	    ',',  "comma",       '-',  "minus",
	    '.',  "period",      '/',  "slash",
	    ':',  "colon",       ';',  "semicolon",
	    '<',  "less",        '=',  "equal",
	    '>',  "greater",     '?',  "question",
	    '@',  "at",          '[',  "bracketleft",
	    '\\', "backslash",   ',',  "bracketright",
	    '^',  "asciicircum", '_',  "underscore",
	    '`',  "grave",       '{',  "braceleft",
	    '|',  "bar",         '}',  "braceright",
	    '~',  "asciitilde",  0,    0};

int kfmt(Fmt *f) {
	int i, c;

	c = va_arg(f->args, int);
	for (i = 0; xkey[i].s; i++) {
		if (xkey[i].c == c) {
			return fmtprint(f, "<%s>", xkey[i].s);
		}
	}
	return fmtprint(f, "<%c>", c);
}
