#include "sam.h"
#include <stdint.h>

Header   h;
uint8_t  indata[DATASIZE];
uint8_t  outdata[2 * DATASIZE + 3]; /* room for overflow message */
uint8_t *inp;
uint8_t *outp;
uint8_t *outmsg = outdata;
Posn     cmdpt;
Posn     cmdptadv;
Buffer   snarfbuf;
bool     waitack;
bool     noflush;
int      tversion;
bool     outbuffered;

int64_t inlong(void);
int     inshort(void);
int     inmesg(Tmesg);
void    setgenstr(File *, Posn, Posn);

void outstart(Hmesg);
void outsend(void);
void outcopy(int, void *);
void outshort(int);
void outlong(int64_t);

#ifdef DEBUG
char *hname[] = {
    [Hversion] = "Hversion",   [Hbindname] = "Hbindname",
    [Hcurrent] = "Hcurrent",   [Hnewname] = "Hnewname",
    [Hmovname] = "Hmovname",   [Hgrow] = "Hgrow",
    [Hcheck0] = "Hcheck0",     [Hcheck] = "Hcheck",
    [Hunlock] = "Hunlock",     [Hdata] = "Hdata",
    [Horigin] = "Horigin",     [Hunlockfile] = "Hunlockfile",
    [Hsetdot] = "Hsetdot",     [Hgrowdata] = "Hgrowdata",
    [Hmoveto] = "Hmoveto",     [Hclean] = "Hclean",
    [Hdirty] = "Hdirty",       [Hcut] = "Hcut",
    [Hsetpat] = "Hsetpat",     [Hdelname] = "Hdelname",
    [Hclose] = "Hclose",       [Hsetsnarf] = "Hsetsnarf",
    [Hsnarflen] = "Hsnarflen", [Hack] = "Hack",
    [Hexit] = "Hexit",
};

char *tname[] = {
    [Tversion] = "Tversion",   [Tstartcmdfile] = "Tstartcmdfile",
    [Tcheck] = "Tcheck",       [Trequest] = "Trequest",
    [Torigin] = "Torigin",     [Tstartfile] = "Tstartfile",
    [Tworkfile] = "Tworkfile", [Ttype] = "Ttype",
    [Tcut] = "Tcut",           [Tpaste] = "Tpaste",
    [Tsnarf] = "Tsnarf",       [Tstartnewfile] = "Tstartnewfile",
    [Twrite] = "Twrite",       [Tclose] = "Tclose",
    [Tlook] = "Tlook",         [Tsearch] = "Tsearch",
    [Tsend] = "Tsend",         [Tcmd] = "Tcmd",
    [Tdclick] = "Tdclick",     [Tstartsnarf] = "Tstartsnarf",
    [Tsetsnarf] = "Tsetsnarf", [Tack] = "Tack",
    [Texit] = "Texit",
};

void journal(int out, char *s) {
	static int fd = 0;

	if (fd <= 0) {
		fd = creat("/tmp/sam.out", 0666L);
	}
	dprintf(fd, "%s%s\n", out ? "out: " : "in:  ", s);
}

void journaln(int out, int64_t n) {
	char buf[32];
	snprintf(buf, sizeof(buf) - 1, "%" PRId64, n);
	journal(out, buf);
}
#else
#define journal(a, b)
#define journaln(a, b)
#endif

int rcvchar(void) {
	static uint8_t buf[64];
	static int     i, nleft = 0;

	if (nleft <= 0) {
		nleft = read(0, (char *)buf, sizeof buf);
		if (nleft <= 0) {
			return -1;
		}
		i = 0;
	}
	--nleft;
	return buf[i++];
}

int rcv(void) {
	int        c;
	static int state = 0;
	static int count = 0;
	static int i = 0;

	while ((c = rcvchar()) != -1) {
		switch (state) {
		case 0:
			h.type = c;
			state++;
			break;

		case 1:
			h.count0 = c;
			state++;
			break;

		case 2:
			h.count1 = c;
			count = h.count0 | (h.count1 << 8);
			i = 0;
			if (count > DATASIZE) {
				panic("count>DATASIZE");
			}
			if (count == 0) {
				goto zerocount;
			}
			state++;
			break;

		case 3:
			indata[i++] = c;
			if (i == count) {
			zerocount:
				indata[i] = 0;
				state = count = 0;
				return inmesg(h.type);
			}
			break;
		}
	}
	return 0;
}

File *whichfile(int tag) {
	int i;

	for (i = 0; i < file.nused; i++) {
		if (file.filepptr[i]->tag == tag) {
			return file.filepptr[i];
		}
	}
	hiccough((char *)0);
	return 0;
}

int inmesg(Tmesg type) {
	Rune    buf[1025];
	int     i, m;
	int16_t s;
	int64_t l, l1, l2;
	File   *f;
	Posn    p0, p1;
	Range   r;
	String *str;
	char   *c;
	Rune   *rp;

	if (type > TMAX) {
		panic("inmesg");
	}

	journal(0, tname[type]);

	inp = indata;
	switch (type) {
	case Terror:
		panic("rcv error");

	default:
		fprintf(stderr, "unknown type %d\n", type);
		panic("rcv unknown");

	case Tversion:
		tversion = inshort();
		journaln(0, tversion);
		break;

	case Tstartcmdfile:
		l = inlong(); /* for 64-bit pointers */
		journaln(0, l);
		Strdupl(&genstr, samname);
		cmd = newfile();
		cmd->unread = 0;
		outTsv(Hbindname, cmd->tag, l);
		outTs(Hcurrent, cmd->tag);
		logsetname(cmd, &genstr);
		cmd->rasp = listalloc('P');
		cmd->mod = 0;
		if (cmdstr.n) {
			loginsert(cmd, 0L, cmdstr.s, cmdstr.n);
			Strdelete(&cmdstr, 0L, (Posn)cmdstr.n);
		}
		fileupdate(cmd, false, true);
		outT0(Hunlock);
		break;

	case Tcheck:
		/* go through whichfile to check the tag */
		outTs(Hcheck, whichfile(inshort())->tag);
		break;

	case Trequest:
		f = whichfile(inshort());
		p0 = inlong();
		p1 = p0 + inshort();
		journaln(0, p0);
		journaln(0, p1 - p0);
		if (f->unread) {
			panic("Trequest: unread");
		}
		if (p1 > f->buf.nc) {
			p1 = f->buf.nc;
		}
		if (p0 >
		    f->buf.nc) { /* can happen e.g. scrolling during command */
			p0 = f->buf.nc;
		}
		if (p0 == p1) {
			i = 0;
			r.p1 = r.p2 = p0;
		} else {
			r = rdata(f->rasp, p0, p1 - p0);
			i = r.p2 - r.p1;
			bufread(&f->buf, r.p1, buf, i);
		}
		buf[i] = 0;
		outTslS(Hdata, f->tag, r.p1, tmprstr(buf, i + 1));
		break;

	case Torigin:
		s = inshort(); /* tag */
		l = inlong();  /* position */
		l1 = inlong(); /* lines to seek past position */
		journaln(0, l1);
		l2 = inlong(); /* cookie to return (identifies layer) */
		journaln(0, l2);
		lookorigin(whichfile(s), l, l1, l2);
		break;

	case Tstartfile:
		termlocked++;
		f = whichfile(inshort());
		if (!f->rasp) { /* this might be a duplicate message */
			f->rasp = listalloc('P');
		}
		current(f);
		outTsv(Hbindname, f->tag, inlong()); /* for 64-bit pointers */
		outTs(Hcurrent, f->tag);
		journaln(0, f->tag);
		if (f->unread) {
			load(f);
		} else {
			if (f->buf.nc > 0) {
				rgrow(f->rasp, 0L, f->buf.nc);
				outTsll(Hgrow, f->tag, 0L, f->buf.nc);
			}
			outTs(Hcheck0, f->tag);
			moveto(f, f->dot.r);
		}
		break;

	case Tworkfile:
		i = inshort();
		f = whichfile(i);
		current(f);
		f->dot.r.p1 = inlong();
		f->dot.r.p2 = inlong();
		f->tdot = f->dot.r;
		journaln(0, i);
		journaln(0, f->dot.r.p1);
		journaln(0, f->dot.r.p2);
		break;

	case Ttype:
		f = whichfile(inshort());
		p0 = inlong();
		journaln(0, p0);
		journal(0, (char *)inp);
		str = tmpcstr((char *)inp);
		i = str->n;
		loginsert(f, p0, str->s, str->n);
		if (fileupdate(f, false, false)) {
			seq++;
		}
		if (f == cmd && p0 == f->buf.nc - i && i > 0 &&
		    str->s[i - 1] == '\n') {
			freetmpstr(str);
			termlocked++;
			termcommand();
		} else {
			freetmpstr(str);
		}
		f->dot.r.p1 = f->dot.r.p2 =
		    p0 + i; /* terminal knows this already */
		f->tdot = f->dot.r;
		break;

	case Tcut:
		f = whichfile(inshort());
		p0 = inlong();
		p1 = inlong();
		journaln(0, p0);
		journaln(0, p1);
		logdelete(f, p0, p1);
		if (fileupdate(f, false, false)) {
			seq++;
		}
		f->dot.r.p1 = f->dot.r.p2 = p0;
		f->tdot =
		    f->dot.r; /* terminal knows the value of dot already */
		break;

	case Tpaste:
		f = whichfile(inshort());
		p0 = inlong();
		journaln(0, p0);
		for (l = 0; l < snarfbuf.nc; l += m) {
			m = snarfbuf.nc - l;
			if (m > BLOCKSIZE) {
				m = BLOCKSIZE;
			}
			bufread(&snarfbuf, l, genbuf, m);
			loginsert(f, p0, tmprstr(genbuf, m)->s, m);
		}
		if (fileupdate(f, false, true)) {
			seq++;
		}
		f->dot.r.p1 = p0;
		f->dot.r.p2 = p0 + snarfbuf.nc;
		f->tdot.p1 = -1; /* force telldot to tell (arguably a BUG) */
		telldot(f);
		outTs(Hunlockfile, f->tag);
		break;

	case Tsnarf:
		i = inshort();
		p0 = inlong();
		p1 = inlong();
		snarf(whichfile(i), p0, p1, &snarfbuf, 0);
		break;

	case Tstartnewfile:
		l = inlong();
		Strdupl(&genstr, empty);
		f = newfile();
		f->rasp = listalloc('P');
		outTsv(Hbindname, f->tag, l);
		logsetname(f, &genstr);
		outTs(Hcurrent, f->tag);
		current(f);
		load(f);
		break;

	case Twrite:
		termlocked++;
		i = inshort();
		journaln(0, i);
		f = whichfile(i);
		addr.r.p1 = 0;
		addr.r.p2 = f->buf.nc;
		if (f->name.s[0] == 0) {
			error(Enoname);
		}
		if (f != cmd) {
			Strduplstr(&genstr, &f->name);
			writef(f);
		}
		break;

	case Tclose:
		termlocked++;
		i = inshort();
		journaln(0, i);
		f = whichfile(i);
		current(f);
		trytoclose(f);
		/* if trytoclose fails, will error out */
		delete (f);
		break;

	case Tlook:
		f = whichfile(inshort());
		termlocked++;
		p0 = inlong();
		p1 = inlong();
		journaln(0, p0);
		journaln(0, p1);
		setgenstr(f, p0, p1);
		for (l = 0; l < genstr.n; l++) {
			i = genstr.s[l];
			if (utfrune(".*+?(|)\\[]^$", i)) {
				str = tmpcstr("\\");
				Strinsert(&genstr, str, l++);
				freetmpstr(str);
			}
		}
		Straddc(&genstr, '\0');
		nextmatch(f, &genstr, p1, 1);
		moveto(f, sel.p[0]);
		break;

	case Tsearch:
		termlocked++;
		if (curfile == 0) {
			error(Enofile);
		}
		if (lastpat.s[0] == 0) {
			panic("Tsearch");
		}
		nextmatch(curfile, &lastpat, curfile->dot.r.p2, 1);
		moveto(curfile, sel.p[0]);
		break;

	case Tsend:
		termlocked++;
		inshort(); /* ignored */
		p0 = inlong();
		p1 = inlong();
		setgenstr(cmd, p0, p1);
		bufreset(&snarfbuf);
		bufinsert(&snarfbuf, (Posn)0, genstr.s, genstr.n);
		outTl(Hsnarflen, genstr.n);
		if (genstr.s[genstr.n - 1] != '\n') {
			Straddc(&genstr, '\n');
		}
		loginsert(cmd, cmd->buf.nc, genstr.s, genstr.n);
		fileupdate(cmd, false, true);
		cmd->dot.r.p1 = cmd->dot.r.p2 = cmd->buf.nc;
		telldot(cmd);
		termcommand();
		break;

	case Tcmd:
		r = curfile->dot.r;
		journal(0, (char *)inp);
		str = tmpcstr((char *)inp);
		runCmdString(str);
		freetmpstr(str);
		if (r.p1 != curfile->dot.r.p1 || r.p2 != curfile->dot.r.p2) {
			moveto(curfile, curfile->dot.r);
		}
		break;

	case Tdclick:
		f = whichfile(inshort());
		p1 = inlong();
		stretchsel(f, p1, false);
		f->tdot.p1 = f->tdot.p2 = p1;
		telldot(f);
		outTs(Hunlockfile, f->tag);
		break;

	case Tstartsnarf:
		if (snarfbuf.nc <= 0) { /* nothing to export */
			outTs(Hsetsnarf, 0);
			break;
		}
		c = 0;
		i = 0;
		m = snarfbuf.nc;
		if (m > SNARFSIZE) {
			m = SNARFSIZE;
			dprint("?warning: snarf buffer truncated\n");
		}
		rp = malloc(m * sizeof(Rune));
		if (rp) {
			bufread(&snarfbuf, 0, rp, m);
			c = Strtoc(tmprstr(rp, m));
			free(rp);
			i = strlen(c);
		}
		outTs(Hsetsnarf, i);
		if (c) {
			Write(stdout, c, i);
			free(c);
		} else {
			dprint("snarf buffer too long\n");
		}
		break;

	case Tsetsnarf:
		m = inshort();
		if (m > SNARFSIZE) {
			error(Etoolong);
		}
		c = malloc(m + 1);
		if (c) {
			for (i = 0; i < m; i++) {
				c[i] = rcvchar();
			}
			c[m] = 0;
			str = tmpcstr(c);
			free(c);
			bufreset(&snarfbuf);
			bufinsert(&snarfbuf, (Posn)0, str->s, str->n);
			freetmpstr(str);
			outT0(Hunlock);
		}
		break;

	case Tack:
		waitack = false;
		break;

	case Texit:
		exit(EXIT_SUCCESS);
	}
	return true;
}

void snarf(File *f, Posn p1, Posn p2, Buffer *buf, bool emptyok) {
	Posn l;
	int  i;

	if (!emptyok && p1 == p2) {
		return;
	}
	bufreset(buf);
	/* Stage through genbuf to avoid compaction problems (vestigial) */
	if (p2 > f->buf.nc) {
		fprintf(stderr, "bad snarf addr p1=%ld p2=%ld f->buf->nc=%d\n",
			p1, p2,
			f->buf.nc); /*ZZZ should never happen, can remove */
		p2 = f->buf.nc;
	}
	for (l = p1; l < p2; l += i) {
		i = p2 - l > BLOCKSIZE ? BLOCKSIZE : p2 - l;
		bufread(&f->buf, l, genbuf, i);
		bufinsert(buf, buf->nc, tmprstr(genbuf, i)->s, i);
	}
}

int inshort(void) {
	uint16_t n;

	n = inp[0] | (inp[1] << 8);
	inp += 2;
	return n;
}

int64_t inlong(void) {
	uint64_t n;

	n = (inp[7] << 24) | (inp[6] << 16) | (inp[5] << 8) | inp[4];
	n = (n << 16) | (inp[3] << 8) | inp[2];
	n = (n << 16) | (inp[1] << 8) | inp[0];
	inp += 8;
	return n;
}

void setgenstr(File *f, Posn p0, Posn p1) {
	if (p0 != p1) {
		if (p1 - p0 >= TBLOCKSIZE) {
			error(Etoolong);
		}
		Strinsure(&genstr, p1 - p0);
		bufread(&f->buf, p0, genbuf, p1 - p0);
		memmove(genstr.s, genbuf, RUNESIZE * (p1 - p0));
		genstr.n = p1 - p0;
	} else {
		if (snarfbuf.nc == 0) {
			error(Eempty);
		}
		if (snarfbuf.nc > TBLOCKSIZE) {
			error(Etoolong);
		}
		bufread(&snarfbuf, (Posn)0, genbuf, snarfbuf.nc);
		Strinsure(&genstr, snarfbuf.nc);
		memmove(genstr.s, genbuf, RUNESIZE * snarfbuf.nc);
		genstr.n = snarfbuf.nc;
	}
}

void outT0(Hmesg type) {
	outstart(type);
	outsend();
}

void outTl(Hmesg type, int64_t l) {
	outstart(type);
	outlong(l);
	outsend();
}

void outTs(Hmesg type, int s) {
	outstart(type);
	journaln(1, s);
	outshort(s);
	outsend();
}

void outS(String *s) {
	char *c;
	int   i;

	c = Strtoc(s);
	i = strlen(c);
	outcopy(i, c);
	if (i > 99) {
		c[99] = 0;
	}
	journaln(1, i);
	journal(1, c);
	free(c);
}

void outTsS(Hmesg type, int s1, String *s) {
	outstart(type);
	outshort(s1);
	outS(s);
	outsend();
}

void outTslS(Hmesg type, int s1, Posn l1, String *s) {
	outstart(type);
	outshort(s1);
	journaln(1, s1);
	outlong(l1);
	journaln(1, l1);
	outS(s);
	outsend();
}

void outTS(Hmesg type, String *s) {
	outstart(type);
	outS(s);
	outsend();
}

void outTsllS(Hmesg type, int s1, Posn l1, Posn l2, String *s) {
	outstart(type);
	outshort(s1);
	outlong(l1);
	outlong(l2);
	journaln(1, l1);
	journaln(1, l2);
	outS(s);
	outsend();
}

void outTsll(Hmesg type, int s, Posn l1, Posn l2) {
	outstart(type);
	outshort(s);
	outlong(l1);
	outlong(l2);
	journaln(1, l1);
	journaln(1, l2);
	outsend();
}

void outTsl(Hmesg type, int s, Posn l) {
	outstart(type);
	outshort(s);
	outlong(l);
	journaln(1, l);
	outsend();
}

void outTsv(Hmesg type, int s, Posn l) {
	outstart(type);
	outshort(s);
	outlong(l);
	journaln(1, l);
	outsend();
}

void outstart(Hmesg type) {
	journal(1, hname[type]);
	outmsg[0] = type;
	outp = outmsg + 3;
}

void outcopy(int count, void *data) {
	memmove(outp, data, count);
	outp += count;
}

void outshort(int s) {
	*outp++ = s;
	*outp++ = s >> 8;
}

void outlong(int64_t l) {
	*outp++ = l;
	*outp++ = l >> 8;
	*outp++ = l >> 16;
	*outp++ = l >> 24;
	*outp++ = l >> 32;
	*outp++ = l >> 40;
	*outp++ = l >> 48;
	*outp++ = l >> 56;
}

void outsend(void) {
	int outcount;

	outcount = outp - outmsg;
	outcount -= 3;
	outmsg[1] = outcount;
	outmsg[2] = outcount >> 8;
	outmsg = outp;
	if (!outbuffered) {
		outcount = outmsg - outdata;
		if (write(STDOUT_FILENO, (char *)outdata, outcount) !=
		    outcount) {
			rescue();
		}
		outmsg = outdata;
		return;
	}
}

int needoutflush(void) { return outmsg >= outdata + DATASIZE; }

void outflush(void) {
	if (outmsg == outdata) {
		return;
	}
	outbuffered = 0;
	/* flow control */
	outT0(Hack);
	waitack = true;
	do {
		if (rcv() == 0) {
			rescue();
			exit(EXIT_FAILURE);
		}
	} while (waitack);
	outmsg = outdata;
	outbuffered = 1;
}
