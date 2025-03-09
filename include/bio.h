#ifndef _BIO_H_
#define _BIO_H_ 1

#ifdef AUTOLIB
AUTOLIB(bio)
#endif

#include <fcntl.h> /* for O_RDONLY, O_WRONLY */
#include <stdarg.h>

typedef struct Biobuf Biobuf;

enum {
	Bsize = 8 * 1024,
	Bungetsize = 4, /* space for ungetc */
	Bmagic = 0x314159,
	Beof = -1,
	Bbad = -2,

	Binactive = 0, /* states */
	Bractive,
	Bwactive,
	Bracteof,

	Bend
};

struct Biobuf {
	int            icount;   /* neg num of bytes at eob */
	int            ocount;   /* num of bytes at bob */
	int            rdline;   /* num of bytes after rdline */
	int            runesize; /* num of bytes of last getrune */
	int            state;    /* r/w/inactive */
	int            fid;      /* open file */
	int            flag;     /* magic if malloc'ed */
	long long      offset;   /* offset of buffer in file */
	int            bsize;    /* size of buffer */
	unsigned char *bbuf;     /* pointer to beginning of buffer */
	unsigned char *ebuf;     /* pointer to end of buffer */
	unsigned char *gbuf;     /* pointer to good data in buf */
	unsigned char  b[Bungetsize + Bsize];
};

int     Bflush(Biobuf *);
int     Binit(Biobuf *, int, int);
int     Binits(Biobuf *, int, int, unsigned char *, int);
Biobuf *Bopen(char *, int);
int     Bprint(Biobuf *, char *, ...);
void   *Brdline(Biobuf *, int);
char   *Brdstr(Biobuf *, int, int);
long    Bread(Biobuf *, void *, long);
int     Bterm(Biobuf *);
long    Bwrite(Biobuf *, void *, long);
int     Bvprint(Biobuf *, char *, va_list);

/* Fits here because it is io into a buffer */
long readn(int f, void *av, long n);

#define OREAD O_RDONLY
#define OWRITE O_WRONLY
#define OTRUNC O_TRUNC
#define OCEXEC O_CLOEXEC
#define ORDWR O_RDWR

#endif
