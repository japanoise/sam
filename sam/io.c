/* Copyright (c) 1998 Lucent Technologies - All rights reserved. */
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>

#include "sam.h"
#include "utf.h"

#define NSYSFILE 3
#define NOFILE 128

#define MIN(x, y) ((x) < (y) ? (x) : (y))

void checkqid(File *f) {
	int   i, w;
	File *g;

	w = whichmenu(f);
	for (i = 1; i < file.nused; i++) {
		g = file.filepptr[i];
		if (w == i) {
			continue;
		}
		if (f->dev == g->dev && f->qidpath == g->qidpath) {
			warn_SS(Wdupfile, &f->name, &g->name);
		}
	}
}

void writef(File *f) {
	Posn     n;
	char    *name;
	int      i, samename, newfile;
	uint64_t dev, qid;
	int64_t  mtime, appendonly, length;

	newfile = 0;
	samename = Strcmp(&genstr, &f->name) == 0;
	name = Strtoc(&f->name);
	i = statfile(name, &dev, &qid, &mtime, 0, 0);
	if (i == -1) {
		newfile++;
	} else if (samename &&
		   (f->dev != dev || f->qidpath != qid || f->mtime < mtime)) {
		f->dev = dev;
		f->qidpath = qid;
		f->mtime = mtime;
		warn_S(Wdate, &genstr);
		free(name);
		return;
	}
	if (genc) {
		free(genc);
	}
	genc = Strtoc(&genstr);
	if ((io = fopen(genc, "w+")) == NULL) {
		error_s(Ecreate, genc);
	}
	dprint("%s: ", genc);
	if (statfd(fileno(io), 0, 0, 0, &length, &appendonly) > 0 &&
	    appendonly && length > 0) {
		error(Eappend);
	}
	n = writeio(f);
	if (f->name.s[0] == 0 || samename) {
		if (addr.r.p1 == 0 && addr.r.p2 == f->buf.nc) {
			f->cleanseq = f->seq;
		}
		state(f, f->cleanseq == f->seq ? Clean : Dirty);
	}
	if (newfile) {
		dprint("(new file) ");
	}
	if (addr.r.p2 > 0 && filereadc(f, addr.r.p2 - 1) != '\n') {
		warn(Wnotnewline);
	}
	closeio(n);
	if (f->name.s[0] == 0 || samename) {
		if (statfile(genc, &dev, &qid, &mtime, 0, 0) > 0) {
			f->dev = dev;
			f->qidpath = qid;
			f->mtime = mtime;
			checkqid(f);
		}
	}
	free(name);
}

Posn readio(File *f, bool *nulls, bool setdate, bool toterm) {
	uint64_t dev, qid;
	int64_t  mtime;
	Posn     nt = 0;
	int      n, b, w;
	Rune    *r;
	Posn     p = addr.r.p2;
	char     buf[BLOCKSIZE + 1], *s;

	*nulls = false;
	b = 0;
	if (f->unread) {
		nt = bufload(&f->buf, 0, fileno(io), nulls);
		if (toterm) {
			raspload(f);
		}
	} else {
		for (nt = 0; (n = read(fileno(io), buf + b, BLOCKSIZE - b)) > 0;
		     nt += (r - genbuf)) {
			n += b;
			b = 0;
			r = genbuf;
			s = buf;
			while (n > 0) {
				if ((*r = *(uint8_t *)s) < Runeself) {
					if (*r) {
						r++;
					} else {
						*nulls = true;
					}
					--n;
					s++;
					continue;
				}
				if (fullrune(s, n)) {
					w = chartorune(r, s);
					if (*r) {
						r++;
					} else {
						*nulls = true;
					}
					n -= w;
					s += w;
					continue;
				}
				b = n;
				memmove(buf, s, b);
				break;
			}
			loginsert(f, p, genbuf, r - genbuf);
		}
	}

	if (b) {
		*nulls = true;
	}
	if (*nulls) {
		warn(Wnulls);
	}
	if (setdate) {
		if (statfd(fileno(io), &dev, &qid, &mtime, 0, 0) > 0) {
			f->dev = dev;
			f->qidpath = qid;
			f->mtime = mtime;
			checkqid(f);
		}
	}
	return nt;
}

void flushio(void) {
	if (io) {
		fflush(io);
	}
}

Posn writeio(File *f) {
	int   m, n;
	Posn  p = addr.r.p1;
	char *c;

	while (p < addr.r.p2) {
		if (addr.r.p2 - p > BLOCKSIZE) {
			n = BLOCKSIZE;
		} else {
			n = addr.r.p2 - p;
		}
		bufread(&f->buf, p, genbuf, n);
		c = Strtoc(tmprstr(genbuf, n));
		m = strlen(c);
		if (Write(io, c, m) != m) {
			free(c);
			if (p > 0) {
				p += n;
			}
			break;
		}
		free(c);
		p += n;
	}
	return p - addr.r.p1;
}

void closeio(Posn p) {
	fclose(io);
	io = NULL;
	if (p >= 0) {
		dprint("#%lu\n", p);
	}
}

char exname[PATH_MAX + 1];
int  remotefd0 = 0;
int  remotefd1 = 1;
int  exfd = -1;

void bootterm(char *machine) {
	char fd[100];
	int  ph2t[2], pt2h[2];

	snprintf(fd, sizeof(fd) - 1, "%d", exfd);

	if (machine) {
		dup2(remotefd0, 0);
		dup2(remotefd1, 1);
		close(remotefd0);
		close(remotefd1);
		if (exfd >= 0) {
			execlp(samterm, samterm, "-r", machine, "-f", fd, "-n",
			       exname, NULL);
		} else {
			execlp(samterm, samterm, "-r", machine, NULL);
		}
		perror("couldn't exec samterm");
		exit(EXIT_FAILURE);
	}

	if (pipe(ph2t) == -1 || pipe(pt2h) == -1) {
		panic("pipe");
	}

	machine = machine ? machine : "localhost";
	switch (fork()) {
	case 0:
		dup2(ph2t[0], 0);
		dup2(pt2h[1], 1);
		close(ph2t[0]);
		close(ph2t[1]);
		close(pt2h[0]);
		close(pt2h[1]);
		if (exfd >= 0) {
			execlp(samterm, samterm, "-r", machine, "-f", fd, "-n",
			       exname, NULL);
		} else {
			execlp(samterm, samterm, "-r", machine, NULL);
		}
		perror("couldn't exec samterm");
		exit(EXIT_FAILURE);
		break;

	case -1:
		panic("can't fork samterm");
		break;
	}

	dup2(pt2h[0], 0);
	dup2(ph2t[1], 1);
	close(ph2t[0]);
	close(ph2t[1]);
	close(pt2h[0]);
	close(pt2h[1]);
}

void connectto(char *machine) {
	int  p1[2], p2[2];
	char sockname[FILENAME_MAX + 1] = {0};
	char rarg[FILENAME_MAX + 1] = {0};

	snprintf(sockname, FILENAME_MAX, "%s/sam.remote.%s",
		 getenv("RSAMSOCKETPATH") ? getenv("RSAMSOCKETPATH") : "/tmp",
		 getuser());

	snprintf(rarg, FILENAME_MAX, "%s:%s", sockname, exname);

	if (pipe(p1) < 0 || pipe(p2) < 0) {
		dprint("can't pipe\n");
		exit(EXIT_FAILURE);
	}
	remotefd0 = p1[0];
	remotefd1 = p2[1];
	switch (fork()) {
	case 0:
		dup2(p2[0], 0);
		dup2(p1[1], 1);
		close(p1[0]);
		close(p1[1]);
		close(p2[0]);
		close(p2[1]);
		execlp(getenv("RSH") ? getenv("RSH") : RXPATH,
		       getenv("RSH") ? getenv("RSH") : RXPATH, "-R", rarg,
		       machine, rsamname, "-R", sockname, NULL);
		dprint("can't exec %s\n", RXPATH);
		exit(EXIT_FAILURE);

	case -1:
		dprint("can't fork\n");
		exit(EXIT_FAILURE);
	}
	close(p1[1]);
	close(p2[0]);
}

char lockpath[FILENAME_MAX + 1] = {0};
int  lockfd = -1;

void removesocket(void) {
	close(exfd);
	unlink(exname);
	exname[0] = 0;

	close(lockfd);
	unlink(lockpath);
	lockpath[0] = 0;
}

bool canlocksocket(const char *machine) {
	const char *path =
	    getenv("SAMSOCKPATH") ? getenv("SAMSOCKPATH") : getenv("HOME");

	if (!path) {
		fputs("could not determine command socket path\n", stderr);
		return true;
	}

	snprintf(lockpath, PATH_MAX, "%s/.sam.%s.lock", path,
		 machine ? machine : "localhost");
	lockfd = open(lockpath, O_CREAT | O_RDWR, 0644);
	if (lockfd < 0) {
		return false;
	}

	if (lockf(lockfd, F_TLOCK, 0) != 0) {
		return close(lockfd), false;
	}

	return true;
}

void opensocket(const char *machine) {
	struct sockaddr_un un = {0};
	const char        *path =
            getenv("SAMSOCKPATH") ? getenv("SAMSOCKPATH") : getenv("HOME");

	if (!canlocksocket(machine)) {
		fputs("could not lock socket\n", stderr);
		return;
	}

	if (!path) {
		fputs("could not determine command socket path\n", stderr);
		return;
	}

	snprintf(exname, PATH_MAX, "%s/.sam.%s", path,
		 machine ? machine : "localhost");

	if (strlen(exname) >= sizeof(un.sun_path) - 1) {
		fputs("command socket path too long\n", stderr);
		return;
	}

	un.sun_family = AF_UNIX;
	strncpy(un.sun_path, exname, sizeof(un.sun_path) - 1);
	if ((exfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0 ||
	    bind(exfd, (struct sockaddr *)&un, sizeof(un)) < 0 ||
	    listen(exfd, 10) < 0) {
		perror("could not open command socket");
		exfd = -1;
		return;
	}

	atexit(removesocket);
}

void startup(char *machine, bool rflag, bool trylock) {
	if (!rflag && trylock) {
		opensocket(machine);
	}

	if (machine) {
		connectto(machine);
	}

	if (!rflag) {
		bootterm(machine);
	}

	downloaded = true;
	outTs(Hversion, VERSION);
}
