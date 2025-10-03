#ifndef _SAM_H
#define _SAM_H
/* Copyright (c) 1998 Lucent Technologies - All rights reserved. */
#include <fcntl.h>
#include <stdint.h>
#include <u.h>
#include "errors.h"

/*
 * BLOCKSIZE is relatively small to keep memory consumption down.
 */

#define BLOCKSIZE 2048
#define RUNESIZE sizeof(Rune)
#define NDISC 5
#define NBUFFILES 3 + 2 * NDISC /* plan 9+undo+snarf+NDISC*(transcript+buf) */
#define NSUBEXP 10

#define INFINITY 0x7FFFFFFFL
#define INCR 25
#define STRSIZE (2 * BLOCKSIZE)

typedef int64_t  Posn; /* file position or address */
typedef uint16_t Mod;  /* modification number */

typedef unsigned int uint;

typedef struct Address  Address;
typedef struct Block    Block;
typedef struct Buffer   Buffer;
typedef struct Disk     Disk;
typedef struct Discdesc Discdesc;
typedef struct File     File;
typedef struct List     List;
typedef struct Range    Range;
typedef struct Rangeset Rangeset;
typedef struct String   String;

enum State {
	Clean = ' ',
	Dirty = '\'',
	Unread = '-',
};

struct Range {
	Posn p1, p2;
};

struct Rangeset {
	Range p[NSUBEXP];
};

struct Address {
	Range r;
	File *f;
};

struct String {
	int16_t n;
	int16_t size;
	Rune   *s;
};

struct List {
	int type; /* 'p' for pointer, 'P' for Posn */
	int nalloc;
	int nused;

	union {
		void    *listp;
		void   **voidp;
		Posn    *posnp;
		String **stringp;
		File   **filep;
	} g;
};

#define listptr g.listp
#define voidpptr g.voidp
#define posnptr g.posnp
#define stringpptr g.stringp
#define filepptr g.filep

enum {
	Blockincr = 256,
	Maxblock = 8 * 1024,

	BUFSIZE = Maxblock, /* size from fbufalloc() */
	RBUFSIZE = BUFSIZE / sizeof(Rune),
};

enum {
	Null = '-',
	Delete = 'd',
	Insert = 'i',
	Filename = 'f',
	Dot = 'D',
	Mark = 'm',
};

struct Block {
	int64_t addr; /* disk address in bytes */

	union {
		uint   n;    /* number of used runes in block */
		Block *next; /* pointer to next in free list */
	};
};

struct Disk {
	int      fd;
	uint64_t addr; /* length of temp file */
	Block   *free[Maxblock / Blockincr + 1];
};

Disk  *diskinit(void);
Block *disknewblock(Disk *, uint);
void   diskrelease(Disk *, Block *);
void   diskread(Disk *, Block *, Rune *, uint);
void   diskwrite(Disk *, Block **, Rune *, uint);

struct Buffer {
	uint    nc;
	Rune   *c;      /* cache */
	uint    cnc;    /* bytes in cache */
	uint    cmax;   /* size of allocated cache */
	uint    cq;     /* position of cache */
	int     cdirty; /* cache needs to be written */
	uint    cbi;    /* index of cache Block */
	Block **bl;     /* array of blocks */
	uint    nbl;    /* number of blocks */
};

void bufinsert(Buffer *, uint, Rune *, uint);
void bufdelete(Buffer *, uint, uint);
uint bufload(Buffer *, uint, int, bool *);
void bufread(Buffer *, uint, Rune *, uint);
void bufclose(Buffer *);
void bufreset(Buffer *);

struct File {
	Buffer   buf;     /* the data */
	Buffer   delta;   /* transcript of changes */
	Buffer   epsilon; /* inversion of delta for redo */
	String   name;    /* name of associated file */
	uint64_t qidpath; /* of file when read */
	uint     mtime;   /* of file when read */
	uint64_t dev;     /* of file when read */
	int      unread;  /* file has not been read from disk */

	int64_t seq;      /* if seq==0, File acts like Buffer */
	int64_t cleanseq; /* f->seq at last read/write of file */
	int     mod;      /* file appears modified in menu */
	char    rescuing; /* sam exiting; this file unusable */

	//	Text		*curtext;	/* most recently used associated
	// text */ 	Text		**text;		/* list of associated
	// texts */ 	int		ntext; 	int		dumpid;
	// /* used
	// in dumping zeroxed windows */

	Posn    hiposn;  /* highest address touched this Mod */
	Address dot;     /* current position */
	Address ndot;    /* new current position after update */
	Range   tdot;    /* what terminal thinks is current range */
	Range   mark;    /* tagged spot in text (don't confuse with Mark) */
	List   *rasp;    /* map of what terminal's got */
	int16_t tag;     /* for communicating with terminal */
	char    closeok; /* ok to close file? */
	char    deleted; /* delete at completion of command */
	Range   prevdot; /* state before start of change */
	Range   prevmark;
	int64_t prevseq;
	int     prevmod;
};

// File*		fileaddtext(File*, Text*);
void fileclose(File *);
void filedelete(File *, uint, uint);
// void		filedeltext(File*, Text*);
void fileinsert(File *, uint, Rune *, uint);
uint fileload(File *, uint, int, bool *);
void filemark(File *);
void filereset(File *);
void filesetname(File *, String *);
void fileundelete(File *, Buffer *, uint, uint);
void fileuninsert(File *, Buffer *, uint, uint);
void fileunsetname(File *, Buffer *);
void fileundo(File *, int, int, uint *, uint *, int);
int  fileupdate(File *, bool, bool);

int     filereadc(File *, uint);
File   *fileopen(void);
void    loginsert(File *, uint, Rune *, uint);
void    logdelete(File *, uint, uint);
void    logsetname(File *, String *);
int     fileisdirty(File *);
int64_t undoseq(File *, int);
int64_t prevseq(Buffer *);

void raspload(File *);
void raspstart(File *);
void raspdelete(File *, uint, uint, int);
void raspinsert(File *, uint, Rune *, uint, int);
void raspdone(File *, int);
void raspflush(File *);

/*
 * acme fns
 */
void *fbufalloc(void);
void  fbuffree(void *);
uint  min(uint, uint);
void  cvttorunes(char *, int, Rune *, int *, int *, bool *);

#define runemalloc(a) (Rune *)emalloc((a) * sizeof(Rune))
#define runerealloc(a, b) (Rune *)realloc((a), (b) * sizeof(Rune))
#define runemove(a, b, c) memmove((a), (b), (c) * sizeof(Rune))

int   alnum(int);
int   Read(FILE *, void *, int);
void  Seek(FILE *, int64_t, int);
int   plan9(File *, int, String *, int);
int   Write(FILE *, void *, int);
int   bexecute(File *, Posn);
bool  canlocksocket(const char *);
void  cd(String *);
void  closefiles(File *, String *);
void  closeio(Posn);
void  cmdloop(void);
void  cmdupdate(void);
void  compile(String *);
void  copy(File *, Address);
File *current(File *);
void delete(File *);
void    delfile(File *);
void    dellist(List *, int);
void    stretchsel(File *, Posn, int);
void    dprint(char *, ...);
void    edit(File *, int);
void   *emalloc(uint64_t);
void   *erealloc(void *, uint64_t);
void    error(Err);
void    error_c(Err, int);
void    error_r(Err, char *);
void    error_s(Err, char *);
int     execute(File *, Posn, Posn);
int     filematch(File *, String *);
void    filename(File *);
void    fixname(String *);
void    freebufs(void);
void    freecmd(void);
void    freecmdlists(void);
void    fullname(String *);
void    getcurwd(void);
File   *getfile(String *);
int     getname(File *, String *, bool);
int64_t getnum(int);
char   *getuser();
void    hiccough(char *);
void    inslist(List *, int, ...);
Address lineaddr(Posn, Address, int);
List   *listalloc(int);
void    listfree(List *);
void    load(File *);
File   *lookfile(String *, bool);
void    lookorigin(File *, Posn, Posn, int64_t);
int     lookup(int);
int     mkdir_p(const char *path, mode_t mode);
void    move(File *, Address);
void    moveto(File *, Range);
File   *newfile(void);
void    nextmatch(File *, String *, Posn, int);
void    notifyf(void *, char *);
void    panic(char *);
void    printposn(File *, int);
void    print_ss(char *, String *, String *);
void    print_s(char *, String *);
int     rcv(void);
Range   rdata(List *, Posn, Posn);
Posn    readio(File *, bool *, bool, bool);
void    rescue(void);
void    resetcmd(void);
void    resetsys(void);
void    resetxec(void);
void    rgrow(List *, Posn, Posn);
void    runCmdString(String *);
void    samerr(char *);
void    samsave(char *);
void    settempfile(void);
void    scram(void);
int     skipbl(void);
void    snarf(File *, Posn, Posn, Buffer *, bool);
void    sortname(File *);
void    startup(char *, bool, bool);
void    state(File *, int);
int     statfd(int, uint64_t *, uint64_t *, int64_t *, int64_t *, int64_t *);
int   statfile(char *, uint64_t *, uint64_t *, int64_t *, int64_t *, int64_t *);
void  Straddc(String *, int);
void  Strclose(String *);
int   Strcmp(String *, String *);
void  Strdelete(String *, Posn, Posn);
void  Strdupl(String *, Rune *);
void  Strduplstr(String *, String *);
void  Strinit(String *);
void  Strinit0(String *);
void  Strinsert(String *, String *, Posn);
void  Strinsure(String *, uint64_t);
int   Strispre(String *, String *);
void  Strzero(String *);
int   Strlen(Rune *);
char *Strtoc(String *);
void  syserror(char *);
void  telldot(File *);
void  tellpat(void);
String   *tmpcstr(char *);
String   *tmprstr(Rune *, int);
void      freetmpstr(String *);
void      termcommand(void);
void      termwrite(char *);
File     *tofile(String *, int);
void      trytoclose(File *);
void      trytoquit(void);
int       undo(int);
void      update(void);
int       waitfor(int);
void      warn(Warn);
void      warn_s(Warn, char *);
void      warn_SS(Warn, String *, String *);
void      warn_S(Warn, String *);
int       whichmenu(File *);
void      writef(File *);
Posn      writeio(File *);
Discdesc *Dstart(void);

extern Rune  samname[]; /* compiler dependent */
extern Rune *left[];
extern Rune *right[];

extern char  RSAM[]; /* system dependent */
extern char  SAMTERM[];
extern char  HOME[];
extern char  TMPDIR[];
extern char *SH;
extern char *SHPATH;
extern char  RX[];
/* extern char  RXPATH[]; */
extern char SAMSAVECMD[];

/*
 * acme globals
 */
extern int64_t seq;
extern Disk   *disk;

extern char    *rsamname; /* globals */
extern char    *samterm;
extern Rune     genbuf[];
extern char    *genc;
extern FILE    *io;
extern bool     patset;
extern bool     quitok;
extern Address  addr;
extern Buffer   snarfbuf;
extern Buffer   plan9buf;
extern Buffer   cmdbuf;
extern int      cmdbufpos;
extern List     file;
extern List     tempfile;
extern File    *cmd;
extern File    *curfile;
extern File    *lastfile;
extern Mod      modnum;
extern Posn     cmdpt;
extern Posn     cmdptadv;
extern Rangeset sel;
extern String   wd;
extern String   cmdstr;
extern String   genstr;
extern String   lastpat;
extern String   lastregexp;
extern String   plan9cmd;
extern bool     downloaded;
extern bool     eof;
extern bool     bpipeok;
extern bool     panicking;
extern Rune     empty[];
extern int      termlocked;
extern bool     outbuffered;

#include "mesg.h"

void outTs(Hmesg, int);
void outT0(Hmesg);
void outTl(Hmesg, int64_t);
void outTslS(Hmesg, int, int64_t, String *);
void outTS(Hmesg, String *);
void outTsS(Hmesg, int, String *);
void outTsllS(Hmesg, int, int64_t, int64_t, String *);
void outTsll(Hmesg, int, int64_t, int64_t);
void outTsl(Hmesg, int, int64_t);
void outTsv(Hmesg, int, int64_t);
void outflush(void);
int  needoutflush(void);

Posn nlcount(File *f, Posn p0, Posn p1);

char *cleanname(char *name);
#endif /* _SAM_H */
