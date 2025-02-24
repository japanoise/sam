/* Copyright (c) 1998 Lucent Technologies - All rights reserved. */
/* Copyright (c) 2016 Rob King */

#include <stdint.h>
#define _XOPEN_SOURCE 500
#include "sam.h"

#include <libgen.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

Rune	    genbuf[BLOCKSIZE];
FILE	   *io;
bool	    panicking;
bool	    rescuing;
Mod	    modnum;
String	    genstr;
String	    rhs;
String	    wd;
String	    cmdstr;
Rune	    empty[] = {0};
char	   *genc;
File	   *curfile;
File	   *flist;
File	   *cmd;
jmp_buf	    mainloop;
List	    tempfile;
bool	    quitok = true;
bool	    downloaded;
bool	    expandtabs;
bool	    dflag;
char	   *machine;
char	   *home;
bool	    bpipeok;
int	    termlocked;
char	   *samterm = "samterm";
char	   *rsamname = "sam";
char	   *SH = "sh";
char	   *SHPATH = "/bin/sh";
char	   *rmsocketname = NULL;
File	   *lastfile;
Disk	   *disk;
int64_t	    seq;

Rune	    baddir[] = {'<', 'b', 'a', 'd', 'd', 'i', 'r', '>', '\n'};

void	    usage(void);

static void hup(int sig) {
	rescue();
	exit(EXIT_FAILURE);
}

int sammain(int argc, char *argv[]);
int bmain(int argc, char *argv[]);

int main(int argc, char *argv[]) {
	if (strcmp(basename(argv[0]), "B") == 0) {
		return bmain(argc, argv);
	}
	return sammain(argc, argv);
}

#define B_CMD_MAX 4095

const char *getbsocketname(const char *machine) {
	const char *user = getuser();
	const char *path =
	    getenv("SAMSOCKETPATH") ? getenv("SAMSOCKETPATH") : getenv("HOME");
	static char name[FILENAME_MAX + 1] = {0};

	if (getenv("SAMSOCKETNAME")) {
		return getenv("SAMSOCKETNAME");
	}

	if (name[0]) {
		return name;
	}

	snprintf(name, FILENAME_MAX, "%s/.sam.%s", path, machine);
	if (access(name, R_OK) == 0) {
		return name;
	}

	snprintf(name, FILENAME_MAX, "%s/.sam.remote.%s", path, user);
	if (access(name, R_OK) == 0) {
		return name;
	}

	snprintf(name, FILENAME_MAX, "/tmp/sam.remote.%s", user);
	if (access(name, R_OK) == 0) {
		return name;
	}

	return NULL;
}

int bmain(int argc, char *argv[]) {
	int		   fd, o;
	struct sockaddr_un un = {0};
	char		   cmd[B_CMD_MAX + 1] = {0};
	bool		   machineset = false;

	machine = "localhost";
	while ((o = getopt(argc, argv, "r:")) != -1) {
		switch (o) {
		case 'r':
			machine = optarg;
			machineset = true;
			break;

		default:
			return fputs("usage: B [-r MACHINE] FILE...\n", stderr),
			       EXIT_FAILURE;
		}
	}

	if (getbsocketname(machine) == NULL) {
		fputs("could not determine controlling socket name\n", stderr);
		if (!machineset) {
			machine = NULL;
		}
		return sammain(argc, argv);
	}

	argc -= optind;
	argv += optind;

	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	strncpy(un.sun_path, getbsocketname(machine), sizeof(un.sun_path) - 1);
	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0 ||
	    connect(fd, (struct sockaddr *)&un, sizeof(un)) < 0) {
		return perror("could not open socket"), EXIT_FAILURE;
	}

	strncat(cmd, "B ", B_CMD_MAX);
	for (int i = 0; i < argc; i++) {
		char path[FILENAME_MAX + 1];
		if (realpath(argv[i], path) == NULL) {
			perror(argv[i]);
		} else {
			strncat(cmd, " ", B_CMD_MAX);
			strncat(cmd, path, B_CMD_MAX);
		}
	}
	strncat(cmd, "\n", B_CMD_MAX);

	if (write(fd, cmd, strlen(cmd)) <= 0) {
		return perror("could not send command"), EXIT_FAILURE;
	}

	close(fd);
	return EXIT_SUCCESS;
}

void rmsocket(void) {
	if (rmsocketname) {
		unlink(rmsocketname);

		char	    lockpath[FILENAME_MAX + 1] = {0};
		const char *path = getenv("SAMSOCKPATH") ? getenv("SAMSOCKPATH")
							 : getenv("HOME");
		snprintf(lockpath, PATH_MAX, "%s/.sam.localhost.lock", path);
		unlink(lockpath);
	}
}

int sammain(int argc, char *argv[]) {
	bool	trylock = true;
	int	i, o;
	String *t;
	char   *arg[argc + 1], **ap;

	ap = &arg[argc];
	arg[0] = "samterm";
	setlocale(LC_ALL, "");

	while ((o = getopt(argc, argv, "SedR:r:t:s:")) != -1) {
		switch (o) {
		case 'd':
			dflag = true;
			break;

		case 'r':
			machine = optarg;
			break;

		case 'R':
			rmsocketname = optarg;
			atexit(rmsocket);
			break;

		case 't':
			samterm = optarg;
			break;

		case 's':
			rsamname = optarg;
			break;

		case 'S':
			trylock = false;
			break;

		default:
			usage();
		}
	}

	if (trylock && !canlocksocket(machine) && !dflag) {
		return bmain(argc, argv);
	}

	argv += optind;
	argc -= optind;

	Strinit(&cmdstr);
	Strinit0(&lastpat);
	Strinit0(&lastregexp);
	Strinit0(&genstr);
	Strinit0(&rhs);
	Strinit0(&wd);
	Strinit0(&plan9cmd);
	disk = diskinit();

	tempfile.listptr = emalloc(0);
	home = getenv("HOME") ? getenv("HOME") : "/";
	SHPATH = getenv("SHELL") ? getenv("SHELL") : SHPATH;
	SH = basename(SHPATH);
	if (!dflag) {
		startup(machine, rmsocketname != NULL, trylock);
	}
	/* Previously: allocate the buffers */
	/* Doesn't appear to be needed in 9front sam */
	/* Fstart(); */

	getcurwd();

	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, hup);
	signal(SIGPIPE, SIG_IGN);

	if (argc > 0) {
		for (i = 0; i < argc; i++) {
			if (!setjmp(mainloop)) {
				t = tmpcstr(argv[i]);
				Straddc(t, '\0');
				Strduplstr(&genstr, t);
				freetmpstr(t);
				filesetname(newfile(), &genstr);
			}
		}
	} else if (!downloaded) {
		newfile()->unread = false;
	}
	seq++;
	modnum++;
	if (file.nused) {
		current(file.filepptr[0]);
	}

	atexit(scram);
	setjmp(mainloop);
	cmdloop();

	trytoquit(); /* if we already q'ed, quitok will be true */

	exit(EXIT_SUCCESS);
}

void scram(void) {
	freecmd();
	for (int i = 0; i < file.nused; i++) {
		fileclose(file.filepptr[i]);
	}

	if (!downloaded) {
		fileclose(cmd);
	}

	if (genc) {
		free(genc);
	}

	Strclose(&cmdstr);
	Strclose(&lastpat);
	Strclose(&lastregexp);
	Strclose(&genstr);
	Strclose(&rhs);
	Strclose(&wd);
	Strclose(&plan9cmd);

	if (file.listptr) {
		free(file.listptr);
	}

	if (tempfile.listptr) {
		free(tempfile.listptr);
	}

	freecmdlists();
	freebufs();
}

void usage(void) {
	fprintf(stderr, "usage: sam [-r machine] [-dfeS] [-t samterm] [-s "
			"samname] FILE...\n");
	exit(EXIT_FAILURE);
}

void rescue(void) {
	if (rescuing++) {
		return;
	}

	if (io) {
		fclose(io);
		io = NULL;
	}

	for (int i = 0; i < file.nused; i++) {
		char  buf[PATH_MAX] = {0};
		File *f = file.filepptr[i];
		if (f == cmd || f->buf.nc == 0 || fileisdirty(f)) {
			continue;
		}

		samsave(buf);
		if ((io = fopen(buf, "w")) == NULL) {
			continue;
		}

		fprintf(io, "samsave() {\n"
			    "    echo \"${1}?\"\n"
			    "    read yn < /dev/tty\n"
			    "    case \"${yn}\" in\n"
			    "        [Yy]*) cat > \"${1}\"\n"
			    "    esac\n"
			    "}\n");

		if (f->name.s[0]) {
			char *c = Strtoc(&f->name);
			snprintf(buf, FILENAME_MAX, "%s", c);
			free(c);
		} else {
			snprintf(buf, FILENAME_MAX, "nameless.%d", i);
		}

		fprintf(io, "samsave %s <<'---%s'\n", buf, buf);
		addr.r.p1 = 0, addr.r.p2 = f->buf.nc;
		writeio(f);
		fprintf(io, "\n---%s\n", (char *)buf);
	}
}

void panic(char *s) {
	int wasd;

	if (!panicking++ && !setjmp(mainloop)) {
		wasd = downloaded;
		downloaded = 0;
		dprint("sam: panic: %s: %r\n", s);
		if (wasd) {
			fprintf(stderr, "sam: panic: %s\n", s);
		}
		rescue();
		abort();
	}
}

void hiccough(char *s) {
	File *f;
	int   i;

	if (rescuing) {
		exit(EXIT_FAILURE);
	}

	if (s) {
		dprint("%s\n", s);
	}

	resetcmd();
	resetxec();
	resetsys();

	if (io) {
		fclose(io);
		io = NULL;
	}

	/*
	 * back out any logged changes & restore old sequences
	 */
	for (i = 0; i < file.nused; i++) {
		f = file.filepptr[i];
		if (f == cmd) {
			continue;
		}
		if (f->seq == seq) {
			bufdelete(&f->epsilon, 0, f->epsilon.nc);
			f->seq = f->prevseq;
			f->dot.r = f->prevdot;
			f->mark = f->prevmark;
			state(f, f->prevmod ? Dirty : Clean);
		}
	}

	update();

	if (curfile) {
		if (curfile->unread) {
			curfile->unread = false;
		} else if (downloaded) {
			outTs(Hcurrent, curfile->tag);
		}
	}

	longjmp(mainloop, 1);
}

void intr(void) { error(Eintr); }

void trytoclose(File *f) {
	char *t;
	char  buf[256];

	if (f == cmd) { /* possible? */
		return;
	}
	if (f->deleted) {
		return;
	}
	if (fileisdirty(f) && !f->closeok) {
		f->closeok = true;
		if (f->name.s[0]) {
			t = Strtoc(&f->name);
			strncpy(buf, t, sizeof buf - 1);
			free(t);
		} else {
			strcpy(buf, "nameless file");
		}
		error_s(Emodified, buf);
	}
	f->deleted = true;
}

void trytoquit(void) {
	int   c;
	File *f;

	if (!quitok) {
		for (c = 0; c < file.nused; c++) {
			f = file.filepptr[c];
			if (f != cmd && fileisdirty(f)) {
				quitok = true;
				eof = false;
				error(Echanges);
			}
		}
	}
}

void load(File *f) {
	Address saveaddr;

	Strduplstr(&genstr, &f->name);
	filename(f);
	if (f->name.s[0]) {
		saveaddr = addr;
		edit(f, 'I');
		addr = saveaddr;
	} else {
		f->unread = 0;
		f->cleanseq = f->seq;
	}

	fileupdate(f, true, true);
}

void cmdupdate(void) {
	if (cmd && cmd->seq != 0) {
		fileupdate(cmd, false, downloaded);
		cmd->dot.r.p1 = cmd->dot.r.p2 = cmd->buf.nc;
		telldot(cmd);
	}
}

void delete(File *f) {
	if (downloaded && f->rasp) {
		outTs(Hclose, f->tag);
	}
	delfile(f);
	if (f == curfile) {
		current(0);
	}
}

void update(void) {
	int   i, anymod;
	File *f;

	settempfile();
	for (anymod = i = 0; i < tempfile.nused; i++) {
		f = tempfile.filepptr[i];
		if (f == cmd) { /* cmd gets done in main() */
			continue;
		}
		if (f->deleted) {
			delete (f);
			continue;
		}
		if (f->seq == seq && fileupdate(f, false, downloaded)) {
			anymod++;
		}
		if (f->rasp) {
			telldot(f);
		}
	}
	if (anymod) {
		seq++;
	}
}

File *current(File *f) { return curfile = f; }

void  edit(File *f, int cmd) {
	 int  empty = true;
	 Posn p;
	 bool nulls;

	 if (cmd == 'r') {
		 logdelete(f, addr.r.p1, addr.r.p2);
	 }
	 if (cmd == 'e' || cmd == 'I') {
		 logdelete(f, (Posn)0, f->buf.nc);
		 addr.r.p2 = f->buf.nc;
	 } else if (f->buf.nc != 0 ||
		    (f->name.s[0] && Strcmp(&genstr, &f->name) != 0)) {
		 empty = false;
	 }
	 if ((io = fopen(genc, "r")) == NULL) {
		 if (curfile && curfile->unread) {
			 curfile->unread = false;
		 }
		 error_r(Eopen, genc);
	 }
	 p = readio(f, &nulls, empty, true);
	 closeio((cmd == 'e' || cmd == 'I') ? -1 : p);
	 if (cmd == 'r') {
		 f->ndot.r.p1 = addr.r.p2, f->ndot.r.p2 = addr.r.p2 + p;
	 } else {
		 f->ndot.r.p1 = f->ndot.r.p2 = 0;
	 }
	 f->closeok = empty;
	 if (quitok) {
		 quitok = empty;
	 } else {
		 quitok = false;
	 }
	 state(f, empty && !nulls ? Clean : Dirty);
	 if (empty && !nulls) {
		 f->cleanseq = f->seq;
	 }
	 if (cmd == 'e') {
		 filename(f);
	 }
}

int getname(File *f, String *s, bool save) {
	int c, i;

	Strzero(&genstr);
	if (genc) {
		free(genc);
		genc = 0;
	}
	if (s == 0 || (c = s->s[0]) == 0) { /* no name provided */
		if (f) {
			Strduplstr(&genstr, &f->name);
		} else {
			Straddc(&genstr, '\0');
		}
		goto Return;
	}
	if (c != ' ' && c != '\t') {
		error(Eblank);
	}
	for (i = 0; (c = s->s[i]) == ' ' || c == '\t'; i++)
		;

	while (s->s[i] > ' ' && i < s->n) {
		if (s->s[i] == '\\') {
			i++;
			if (i >= s->n) {
				break;
			}
		}
		Straddc(&genstr, s->s[i++]);
	}

	if (s->s[i]) {
		error(Enewline);
	}
	fixname(&genstr);
	if (f && (save || f->name.s[0] == 0)) {
		logsetname(f, &genstr);
		if (Strcmp(&f->name, &genstr)) {
			quitok = f->closeok = false;
			f->qidpath = 0;
			f->mtime = 0;
			state(f, Dirty); /* if it's 'e', fix later */
		}
	}
Return:
	genc = Strtoc(&genstr);
	i = genstr.n;
	if (i && genstr.s[i - 1] == 0) {
		i--;
	}
	return i; /* strlen(name) */
}

void filename(File *f) {
	if (genc) {
		free(genc);
	}
	genc = Strtoc(&genstr);
	dprint("%c%c%c %s\n", " '"[f->mod], "-+"[f->rasp != 0],
	       " ."[f == curfile], genc);
}

void undostep(File *f, int isundo) {
	uint p1, p2;
	int  mod;

	mod = f->mod;
	fileundo(f, isundo, 1, &p1, &p2, true);
	f->ndot = f->dot;
	if (f->mod) {
		f->closeok = 0;
		quitok = 0;
	} else {
		f->closeok = 1;
	}

	if (f->mod != mod) {
		f->mod = mod;
		if (mod) {
			mod = Clean;
		} else {
			mod = Dirty;
		}
		state(f, mod);
	}
}

int undo(int isundo) {
	File *f;
	int   i;
	Mod   max;

	max = undoseq(curfile, isundo);
	if (max == 0) {
		return 0;
	}
	settempfile();
	for (i = 0; i < tempfile.nused; i++) {
		f = tempfile.filepptr[i];
		if (f != cmd && undoseq(f, isundo) == max) {
			undostep(f, isundo);
		}
	}
	return 1;
}

int readcmd(String *s) {
	int retcode;

	if (flist != 0) {
		fileclose(flist);
	}
	flist = fileopen();

	addr.r.p1 = 0, addr.r.p2 = flist->buf.nc;
	retcode = plan9(flist, '<', s, false);
	fileupdate(flist, false, false);
	flist->seq = 0;
	if (flist->buf.nc > BLOCKSIZE) {
		error(Etoolong);
	}
	Strzero(&genstr);
	Strinsure(&genstr, flist->buf.nc);
	bufread(&flist->buf, (Posn)0, genbuf, flist->buf.nc);
	memmove(genstr.s, genbuf, flist->buf.nc * RUNESIZE);
	genstr.n = flist->buf.nc;
	Straddc(&genstr, '\0');
	return retcode;
}

void getcurwd(void) {
	String *t;
	char	buf[PATH_MAX + 1];

	buf[0] = 0;
	getcwd(buf, sizeof(buf));
	t = tmpcstr(buf);
	Strduplstr(&wd, t);
	freetmpstr(t);
	if (wd.n == 0) {
		warn(Wpwd);
	} else if (wd.s[wd.n - 1] != '/') {
		Straddc(&wd, '/');
	}
}

void cd(String *str) {
	int	i;
	File   *f;
	String *t;
	String	owd;

	t = tmpcstr("/bin/pwd");
	Straddc(t, '\0');
	if (flist) {
		fileclose(flist);
		flist = 0;
	}
	if (readcmd(t) != 0) {
		Strduplstr(&genstr,
			   tmprstr(baddir, sizeof(baddir) / sizeof(Rune)));
		Straddc(&genstr, '\0');
	}
	freetmpstr(t);

	getcurwd();
	if (chdir(getname((File *)0, str, false) ? genc : home)) {
		syserror("chdir");
	}
	settempfile();
	for (i = 0; i < tempfile.nused; i++) {
		f = tempfile.filepptr[i];
		if (f != cmd && f->name.s[0] != '/' && f->name.s[0] != 0) {
			Strinsert(&f->name, &owd, (Posn)0);
			fixname(&f->name);
			sortname(f);
		} else if (f != cmd && Strispre(&wd, &f->name)) {
			fixname(&f->name);
			sortname(f);
		}
	}
	Strclose(&owd);
}

int loadflist(String *s, int blank) {
	int c, i;

	c = s->s[0];
	for (i = 0; i < s->n && (s->s[i] == ' ' || s->s[i] == '\t'); i++)
		;
	if (blank == 0 || ((c == ' ' || c == '\t') && s->s[i] != '\n')) {
		if (s->s[i] == '<') {
			Strdelete(s, 0L, (int64_t)i + 1);
			readcmd(s);
		} else {
			Strzero(&genstr);
			while (i < s->n && (c = s->s[i++]) && c != '\n') {
				Straddc(&genstr, c);
			}
			Straddc(&genstr, '\0');
		}
	} else {
		if (c != '\n') {
			error(Eblank);
		}
		Strdupl(&genstr, empty);
	}
	if (genc) {
		free(genc);
	}
	genc = Strtoc(&genstr);
	return genstr.s[0];
}

File *readflist(bool readall, bool delete) {
	Posn   i;
	Rune   c;
	File  *f;
	String t;

	for (i = 0, f = NULL; f == NULL || readall || delete;
	     i++) { /* ++ skips blank */
		bool esc = false;
		Strinit(&t);
		while (i < genstr.n && ((c = genstr.s[i]) == L' ' ||
					c == L'\t' || c == L'\n')) {
			i++;
		}

		if (i >= genstr.n) {
			break;
		}

		while (i < genstr.n) {
			c = genstr.s[i];
			if (esc) {
				esc = false;
			} else if (c == L'\\') {
				esc = true;
				i++;
				continue;
			} else if (c == L' ' || c == L'\t' || c == L'\n' ||
				   c == 0) {
				break;
			}
			Straddc(&t, c);
			i++;
		}

		Straddc(&t, 0);
		f = lookfile(&t, false);
		if (delete) {
			if (f == NULL) {
				warn_S(Wfile, &t);
			} else {
				trytoclose(f);
			}
		} else if (f == NULL && readall) {
			logsetname(f = newfile(), &t);
		}

		if (i == 0 || i >= genstr.n || t.n == 0) {
			break;
		}

		Strclose(&t);
	}
	if (t.s) {
		Strclose(&t);
	}
	return f;
}

File *tofile(String *s, int blank) {
	File *f = NULL;

	if (blank && s->s[0] != ' ') {
		error(Eblank);
	}

	if (loadflist(s, blank) == 0) {
		f = lookfile(&genstr, false);
	}

	if (f == NULL) {
		f = lookfile(&genstr, true);
	}

	if (f == NULL) {
		f = readflist(false, false);
	}

	if (f == NULL) {
		error_s(Emenu, genc);
	}

	return current(f);
}

File *getfile(String *s) {
	File *f;

	if (loadflist(s, 1) == 0) {
		logsetname(f = newfile(), &genstr);
	} else if ((f = readflist(true, false)) == 0) {
		error(Eblank);
	}
	return current(f);
}

void closefiles(File *f, String *s) {
	if (s->s[0] == 0) {
		if (f == 0) {
			error(Enofile);
		}
		trytoclose(f);
		return;
	}
	if (s->s[0] != ' ') {
		error(Eblank);
	}
	if (loadflist(s, 1) == 0) {
		error(Enewline);
	}
	readflist(false, true);
}

void copy(File *f, Address addr2) {
	Posn p;
	int  ni;
	for (p = addr.r.p1; p < addr.r.p2; p += ni) {
		ni = addr.r.p2 - p;
		if (ni > BLOCKSIZE) {
			ni = BLOCKSIZE;
		}
		bufread(&f->buf, p, genbuf, ni);
		loginsert(addr2.f, addr2.r.p2, tmprstr(genbuf, ni)->s, ni);
	}
	addr2.f->ndot.r.p2 = addr2.r.p2 + (f->dot.r.p2 - f->dot.r.p1);
	addr2.f->ndot.r.p1 = addr2.r.p2;
}

void move(File *f, Address addr2) {
	if (addr.r.p2 <= addr2.r.p2) {
		logdelete(f, addr.r.p1, addr.r.p2);
		copy(f, addr2);
	} else if (addr.r.p1 >= addr2.r.p2) {
		copy(f, addr2);
		logdelete(f, addr.r.p1, addr.r.p2);
	} else {
		error(Eoverlap);
	}
}

Posn nlcount(File *f, Posn p0, Posn p1) {
	Posn nl = 0;

	while (p0 < p1) {
		if (filereadc(f, p0++) == '\n') {
			nl++;
		}
	}
	return nl;
}

void printposn(File *f, int chars) {
	Posn  l1, l2;
	char *s;

	if (f->name.s[0]) {
		if (f->name.s[0] != '/') {
			getcurwd();
			s = Strtoc(&wd);
			dprint("%s", s);
			free(s);
		}
		s = Strtoc(&f->name);
		dprint("%s:", s);
		free(s);
	}
	if (chars) {
		dprint("#%lud", addr.r.p1);
		if (addr.r.p2 != addr.r.p1) {
			dprint(",#%lud", addr.r.p2);
		}
	} else {
		l1 = 1 + nlcount(f, (Posn)0, addr.r.p1);
		l2 = l1 + nlcount(f, addr.r.p1, addr.r.p2);
		/* check if addr ends with '\n' */
		if (addr.r.p2 > 0 && addr.r.p2 > addr.r.p1 &&
		    filereadc(f, addr.r.p2 - 1) == '\n') {
			--l2;
		}
		dprint("%lud", l1);
		if (l2 != l1) {
			dprint(",%lud", l2);
		}
	}
	dprint("\n");
}

void settempfile(void) {
	if (tempfile.nalloc < file.nused) {
		if (tempfile.filepptr) {
			free(tempfile.filepptr);
		}
		tempfile.filepptr = emalloc(sizeof(File *) * file.nused);
		tempfile.nalloc = file.nused;
	}
	memmove(tempfile.filepptr, file.filepptr, sizeof(File *) * file.nused);
	tempfile.nused = file.nused;
}
