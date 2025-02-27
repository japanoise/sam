#include "sam.h"
#include <fcntl.h>

static Block *blist;

char buf[128];
Disk d;

static void diskcleanup() {
	close(d.fd);
	remove(buf);
}

static int tempdisk(void) {
	int i, fd;

	snprintf(buf, sizeof buf, "/tmp/X%d.%.4ssam", getpid(), getuser());
	for (i = 'A'; i <= 'Z'; i++) {
		buf[5] = i;
		if (access(buf, F_OK) == 0) {
			continue;
		}
		/* maybe? fd = open(buf, O_RDWR|O_TMPFILE); */
		fd = open(buf, O_RDWR | O_EXCL | O_CREAT);
		if (fd >= 0) {
			return fd;
		}
	}
	return -1;
}

Disk *diskinit() {
	d.fd = tempdisk();
	if (d.fd < 0) {
		fprintf(stderr, "sam: can't create temp file\n");
		exit(EXIT_FAILURE);
	}
	atexit(diskcleanup);
	return &d;
}

static uint ntosize(uint n, uint *ip) {
	uint size;

	if (n > Maxblock) {
		panic("internal error: ntosize");
	}
	size = n;
	if (size & (Blockincr - 1)) {
		size += Blockincr - (size & (Blockincr - 1));
	}
	/* last bucket holds blocks of exactly Maxblock */
	if (ip) {
		*ip = size / Blockincr;
	}
	return size * sizeof(Rune);
}

Block *disknewblock(Disk *d, uint n) {
	uint   i, j, size;
	Block *b;

	size = ntosize(n, &i);
	b = d->free[i];
	if (b) {
		d->free[i] = b->next;
	} else {
		/* allocate in chunks to reduce malloc overhead */
		if (blist == NULL) {
			blist = emalloc(100 * sizeof(Block));
			for (j = 0; j < 100 - 1; j++) {
				blist[j].next = &blist[j + 1];
			}
		}
		b = blist;
		blist = b->next;
		b->addr = d->addr;
		if (d->addr + size < d->addr) {
			panic("temp file overflow");
		}
		d->addr += size;
	}
	b->n = n;
	return b;
}

void diskrelease(Disk *d, Block *b) {
	uint i;

	ntosize(b->n, &i);
	b->next = d->free[i];
	d->free[i] = b;
}

void diskwrite(Disk *d, Block **bp, Rune *r, uint n) {
	int    size, nsize;
	Block *b;

	b = *bp;
	size = ntosize(b->n, NULL);
	nsize = ntosize(n, NULL);
	if (size != nsize) {
		diskrelease(d, b);
		b = disknewblock(d, n);
		*bp = b;
	}
	if (pwrite(d->fd, r, n * sizeof(Rune), b->addr) != n * sizeof(Rune)) {
		panic("write error to temp file");
	}
	b->n = n;
}

void diskread(Disk *d, Block *b, Rune *r, uint n) {
	if (n > b->n) {
		panic("internal error: diskread");
	}

	ntosize(b->n, NULL); /* called only for sanity check on Maxblock */
	if (pread(d->fd, r, n * sizeof(Rune), b->addr) != n * sizeof(Rune)) {
		panic("read error from temp file");
	}
}
