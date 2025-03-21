/* Copyright (c) 1998 Lucent Technologies - All rights reserved. */
/* Version is transmitted in a uint16, so the maximum is 65535 */
/* thus YYMMr - Y = year, M = month, r = revision that month */
/* Should keep us from needing to increase it, until 2065 :^) */
#define VERSION 25021

#define TBLOCKSIZE 512 /* largest piece of text sent to terminal */
#define DATASIZE                                                               \
	(UTFmax * TBLOCKSIZE + 30) /* ... including protocol header stuff      \
				    */
#define SNARFSIZE 16384 /* maximum length of exchanged snarf buffer */

/*
 * Messages originating at the terminal
 */
typedef enum Tmesg {
	Terror = -1,   /* error */
	Tversion,      /* version */
	Tstartcmdfile, /* terminal just opened command frame */
	Tcheck,        /* ask host to poke with Hcheck */
	Trequest,      /* request data to fill a hole */
	Torigin,       /* gimme an Horigin near here */
	Tstartfile,    /* terminal just opened a file's frame */
	Tworkfile,     /* set file to which commands apply */
	Ttype,         /* add some characters, but terminal already knows */
	Tcut,
	Tpaste,
	Tsnarf,
	Tstartnewfile, /* terminal just opened a new frame */
	Twrite,        /* write file */
	Tclose,        /* terminal requests file close; check mod. status */
	Tlook,         /* search for literal current text */
	Tsearch,       /* search for last regular expression */
	Tsend,         /* pretend he typed stuff */
	Tcmd,          /* run a command */
	Tdclick,       /* double click */
	Tstartsnarf,   /* initiate snarf buffer exchange */
	Tsetsnarf,     /* remember string in snarf buffer */
	Tack,          /* acknowledge Hack */
	Texit,         /* exit */
	TMAX
} Tmesg;

/*
 * Messages originating at the host
 */
typedef enum Hmesg {
	Herror = -1, /* error */
	Hversion,    /* version */
	Hbindname,   /* attach name[0] to text in terminal */
	Hcurrent,    /* make named file the typing file */
	Hnewname,    /* create "" name in menu */
	Hmovname,    /* move file name in menu */
	Hgrow,       /* insert space in rasp */
	Hcheck0,     /* see below */
	Hcheck,      /* ask terminal to check whether it needs more data */
	Hunlock,     /* command is finished; user can do things */
	Hdata,       /* store this data in previously allocated space */
	Horigin,     /* set origin of file/frame in terminal */
	Hunlockfile, /* unlock file in terminal */
	Hsetdot,     /* set dot in terminal */
	Hgrowdata,   /* Hgrow + Hdata folded together */
	Hmoveto,     /* scrolling, context search, etc. */
	Hclean,      /* named file is now 'clean' */
	Hdirty,      /* named file is now 'dirty' */
	Hcut,        /* remove space from rasp */
	Hsetpat,     /* set remembered regular expression */
	Hdelname,    /* delete file name from menu */
	Hclose,      /* close file and remove from menu */
	Hsetsnarf,   /* remember string in snarf buffer */
	Hsnarflen,   /* report length of implicit snarf */
	Hack,        /* request acknowledgement */
	Hexit,
	HMAX
} Hmesg;

typedef struct Header {
	uint8_t type;    /* one of the above */
	uint8_t count0;  /* low bits of data size */
	uint8_t count1;  /* high bits of data size */
	uint8_t data[1]; /* variable size */
} Header;

/*
 * File transfer protocol schematic, a la Holzmann
 *
 *  proc h
 *  {   pvar n = 0;
 *      queue h[4];
 *
 *      do
 *      :: (n <  N)  -> n++; t!Hgrow
 *      :: (n == N)  -> n++; t!Hcheck0
 *      :: h?Trequest -> t!Hdata
 *      :: h?Tcheck  -> t!Hcheck
 *      od
 *  }
 *  proc t
 *  {   queue t[4];
 *      do
 *      :: t?Hgrow -> h!Trequest
 *      :: t?Hdata -> skip
 *      :: t?Hcheck0 -> h!Tcheck
 *      :: t?Hcheck ->
 *          if
 *          :: break
 *          :: h!Trequest; h!Tcheck
 *          fi
 *      od
 *  }
 */
