/* Copyright (c) 2006 Russ Cox */

#include <string.h>
#include <u.h>
#include <sys/select.h>
#include <draw.h>
#include <mouse.h>
#include <cursor.h>
#include <drawfcall.h>
#include <thread.h>
#include <bio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include "mux.h"

char *argv0;

extern Mouse _drawmouse;
int          chattydrawclient = 0;

static int   drawgettag(Mux *mux, void *vmsg);
static void *drawrecv(Mux *mux);
static int   drawnbrecv(Mux *mux, void **);
static int   drawsend(Mux *mux, void *vmsg);
static int   drawsettag(Mux *mux, void *vmsg, uint tag);
static int   canreadfd(int);

/*
 * Absent other hints, it works reasonably well to use
 * the X11 display name as the name space identifier.
 * This is how sam's B has worked since the early days.
 * Since most programs using name spaces are also using X,
 * this still seems reasonable.  Terminal-only sessions
 * can set $NAMESPACE.
 */
static char *nsfromdisplay(void) {
	int         fd;
	struct stat d;
	char       *disp, *p;

	if ((disp = getenv("DISPLAY")) == NULL) {
#ifdef __APPLE__
		// Might be running native GUI on OS X.
		disp = strdup(":0.0");
		if (disp == NULL) {
			return NULL;
		}
#else
		werrstr("$DISPLAY not set");
		return NULL;
#endif
	}

	/* canonicalize: xxx:0.0 => xxx:0 */
	p = strrchr(disp, ':');
	if (p) {
		p++;
		while (isdigit((uchar)*p)) {
			p++;
		}
		if (strcmp(p, ".0") == 0) {
			*p = 0;
		}
	}

	/* turn /tmp/launch/:0 into _tmp_launch_:0 (OS X 10.5) */
	for (p = disp; *p; p++) {
		if (*p == '/') {
			*p = '_';
		}
	}

	p = smprint("/tmp/ns.%d.%s", getuid(), disp);
	free(disp);
	if (p == NULL) {
		werrstr("out of memory");
		return p;
	}
	if (!mkdir(p, 0700) || errno == EEXIST) {
		return p;
	}
	if (stat(p, &d)) {
		int errsrv = errno;
		werrstr("stat %s: %s", p, strerror(errsrv));
		free(p);
		return NULL;
	}
	if ((d.st_mode & 0777) != 0700 || d.st_uid != getuid()) {
		werrstr("bad name space dir %s", p);
		free(p);
		return NULL;
	}
	return p;
}

char *getns(void) {
	char *ns;

	ns = getenv("NAMESPACE");
	if (ns == NULL) {
		ns = nsfromdisplay();
	}
	if (ns == NULL) {
		werrstr("$NAMESPACE not set, %r");
		return NULL;
	}
	return ns;
}

int _displayconnect(Display *d) {
	int     pid, p[2], fd, nbuf, n;
	char   *wsysid, *ns, *addr, *id;
	uchar  *buf;
	Wsysmsg w;

	fmtinstall('W', drawfcallfmt);
	fmtinstall('H', encodefmt);

	wsysid = getenv("wsysid");
	if (wsysid != NULL) {
		// Connect to running devdraw service.
		// wsysid=devdrawname/id
		id = strchr(wsysid, '/');
		if (id == NULL) {
			werrstr("invalid $wsysid");
			return -1;
		}
		*id++ = '\0';
		if ((ns = getns()) == NULL) {
			return -1;
		}
		addr = smprint("unix!%s/%s", ns, wsysid);
		free(ns);
		if (addr == NULL) {
			return -1;
		}
		fd = p9dial(addr, 0, 0, 0);
		free(addr);
		if (fd < 0) {
			return -1;
		}
		nbuf = strlen(id) + 500;
		buf = malloc(nbuf);
		if (buf == NULL) {
			close(fd);
			return -1;
		}
		memset(&w, 0, sizeof w);
		w.type = Tctxt;
		w.id = id;
		n = convW2M(&w, buf, nbuf);
		if (write(fd, buf, n) != n) {
			close(fd);
			werrstr("wsys short write: %r");
			return -1;
		}
		n = readwsysmsg(fd, buf, nbuf);
		if (n < 0) {
			close(fd);
			werrstr("wsys short read: %r");
			return -1;
		}
		if (convM2W(buf, n, &w) <= 0) {
			close(fd);
			werrstr("wsys decode error");
			return -1;
		}
		if (w.type != Rctxt) {
			close(fd);
			if (w.type == Rerror) {
				werrstr("%s", w.error);
			} else {
				werrstr("wsys rpc phase error (%d)", w.type);
			}
			return -1;
		}
		d->srvfd = fd;
		return 0;
	}

	if (pipe(p) < 0) {
		return -1;
	}
	if ((pid = fork()) < 0) {
		close(p[0]);
		close(p[1]);
		return -1;
	}
	if (pid == 0) {
		char *devdraw;

		devdraw = getenv("DEVDRAW");
		if (devdraw == NULL) {
			devdraw = "devdraw";
		}
		close(p[0]);
		p9dup(p[1], 0);
		p9dup(p[1], 1);
		/* execl("strace", "strace", "-o", "drawsrv.out", "drawsrv",
		 * NULL); */
		/*
		 * The argv0 has no meaning to devdraw.
		 * Pass it along only so that the various
		 * devdraws in psu -a can be distinguished.
		 * The NOLIBTHREADDAEMONIZE keeps devdraw from
		 * forking before threadmain. OS X hates it when
		 * guis fork.
		 *
		 * If client didn't use ARGBEGIN, argv0 == NULL.
		 * Can't send NULL through because OS X expects
		 * argv[0] to be non-NULL.  Also, OS X apparently
		 * expects argv[0] to be a valid executable name,
		 * so "(argv0)" is not okay.  Use "devdraw"
		 * instead.
		 */
		putenv("NOLIBTHREADDAEMONIZE=1");
		devdraw = getenv("DEVDRAW");
		if (devdraw == NULL) {
			devdraw = "devdraw";
		}
		if (argv0 == NULL) {
			argv0 = devdraw;
		}
		execl(devdraw, argv0, argv0, "(devdraw)", NULL);
		sysfatal("exec devdraw: %r");
	}
	close(p[1]);
	d->srvfd = p[0];
	return 0;
}

int _displaymux(Display *d) {
	if ((d->mux = calloc(sizeof(*d->mux), 1)) == NULL) {
		return -1;
	}

	d->mux->mintag = 1;
	d->mux->maxtag = 255;
	d->mux->send = drawsend;
	d->mux->recv = drawrecv;
	d->mux->nbrecv = drawnbrecv;
	d->mux->gettag = drawgettag;
	d->mux->settag = drawsettag;
	d->mux->aux = d;
	muxinit(d->mux);

	return 0;
}

static int drawsend(Mux *mux, void *vmsg) {
	int      n;
	uchar   *msg;
	Display *d;

	msg = vmsg;
	GET(msg, n);
	d = mux->aux;
	return write(d->srvfd, msg, n);
}

static int _drawrecv(Mux *mux, int canblock, void **vp) {
	int      n;
	uchar    buf[4], *p;
	Display *d;

	d = mux->aux;
	*vp = NULL;
	if (!canblock && !canreadfd(d->srvfd)) {
		return 0;
	}
	if ((n = readn(d->srvfd, buf, 4)) != 4) {
		return 1;
	}
	GET(buf, n);
	p = malloc(n);
	if (p == NULL) {
		fprint(2, "out of memory allocating %d in drawrecv\n", n);
		return 1;
	}
	memmove(p, buf, 4);
	if (readn(d->srvfd, p + 4, n - 4) != n - 4) {
		free(p);
		return 1;
	}
	*vp = p;
	return 1;
}

static void *drawrecv(Mux *mux) {
	void *p;
	_drawrecv(mux, 1, &p);
	return p;
}

static int drawnbrecv(Mux *mux, void **vp) { return _drawrecv(mux, 0, vp); }

static int drawgettag(Mux *mux, void *vmsg) {
	uchar *msg;

	msg = vmsg;
	return msg[4];
}

static int drawsettag(Mux *mux, void *vmsg, uint tag) {
	uchar *msg;

	msg = vmsg;
	msg[4] = tag;
	return 0;
}

static int displayrpc(Display *d, Wsysmsg *tx, Wsysmsg *rx, void **freep) {
	int   n, nn;
	void *tpkt, *rpkt;

	n = sizeW2M(tx);
	tpkt = malloc(n);
	if (freep) {
		*freep = NULL;
	}
	if (tpkt == NULL) {
		return -1;
	}
	tx->tag = 0;
	if (chattydrawclient) {
		fprint(2, "<- %W\n", tx);
	}
	nn = convW2M(tx, tpkt, n);
	if (nn != n) {
		free(tpkt);
		werrstr("drawclient: sizeW2M convW2M mismatch");
		fprint(2, "%r\n");
		return -1;
	}
	/*
	 * This is the only point where we might reschedule.
	 * Muxrpc might need to acquire d->mux->lk, which could
	 * be held by some other proc (e.g., the one reading from
	 * the keyboard via Trdkbd messages).  If we need to wait
	 * for the lock, don't let other threads from this proc
	 * run.  This keeps up the appearance that writes to /dev/draw
	 * don't cause rescheduling.  If you *do* allow rescheduling
	 * here, then flushimage(display, 1) happening in two different
	 * threads in the same proc can cause a buffer of commands
	 * to be written out twice, leading to interesting results
	 * on the screen.
	 *
	 * Threadpin and threadunpin were added to the thread library
	 * to solve exactly this problem.  Be careful!  They are dangerous.
	 *
	 * _pin and _unpin are aliases for threadpin and threadunpin
	 * in a threaded program and are no-ops in unthreaded programs.
	 *
	 * 25/03/08: sam is always threaded, AFAICT, so replaced these
	 * with threadpin/threadunpin. - japanoise
	 */
	threadpin();
	rpkt = muxrpc(d->mux, tpkt);
	threadunpin();
	free(tpkt);
	if (rpkt == NULL) {
		werrstr("muxrpc: %r");
		return -1;
	}
	GET((uchar *)rpkt, n);
	nn = convM2W(rpkt, n, rx);
	if (nn != n) {
		free(rpkt);
		werrstr("drawclient: convM2W packet size mismatch %d %d %.*H",
			n, nn, n, rpkt);
		fprint(2, "%r\n");
		return -1;
	}
	if (chattydrawclient) {
		fprint(2, "-> %W\n", rx);
	}
	if (rx->type == Rerror) {
		werrstr("%s", rx->error);
		free(rpkt);
		return -1;
	}
	if (rx->type != tx->type + 1) {
		werrstr("packet type mismatch -- tx %d rx %d", tx->type,
			rx->type);
		free(rpkt);
		return -1;
	}
	if (freep) {
		*freep = rpkt;
	} else {
		free(rpkt);
	}
	return 0;
}

int _displayinit(Display *d, char *label, char *winsize) {
	Wsysmsg tx, rx;

	tx.type = Tinit;
	tx.label = label;
	tx.winsize = winsize;
	return displayrpc(d, &tx, &rx, NULL);
}

int _displayrdmouse(Display *d, Mouse *m, int *resized) {
	Wsysmsg tx, rx;

	tx.type = Trdmouse;
	if (displayrpc(d, &tx, &rx, NULL) < 0) {
		return -1;
	}
	_drawmouse = rx.mouse;
	*m = rx.mouse;
	*resized = rx.resized;
	return 0;
}

int _displayrdkbd(Display *d, Rune *r) {
	Wsysmsg tx, rx;

	tx.type = Trdkbd4;
	if (displayrpc(d, &tx, &rx, NULL) < 0) {
		return -1;
	}
	*r = rx.rune;
	return 0;
}

int _displaymoveto(Display *d, Point p) {
	Wsysmsg tx, rx;

	tx.type = Tmoveto;
	tx.mouse.xy = p;
	if (displayrpc(d, &tx, &rx, NULL) < 0) {
		return -1;
	}
	_drawmouse.xy = p;
	return flushimage(d, 1);
}

int _displaycursor(Display *d, Cursor *c, Cursor2 *c2) {
	Wsysmsg tx, rx;

	tx.type = Tcursor2;
	if (c == NULL) {
		memset(&tx.cursor, 0, sizeof tx.cursor);
		memset(&tx.cursor2, 0, sizeof tx.cursor2);
		tx.arrowcursor = 1;
	} else {
		tx.arrowcursor = 0;
		tx.cursor = *c;
		if (c2 != NULL) {
			tx.cursor2 = *c2;
		} else {
			scalecursor(&tx.cursor2, c);
		}
	}
	return displayrpc(d, &tx, &rx, NULL);
}

int _displaybouncemouse(Display *d, Mouse *m) {
	Wsysmsg tx, rx;

	tx.type = Tbouncemouse;
	tx.mouse = *m;
	return displayrpc(d, &tx, &rx, NULL);
}

int _displaylabel(Display *d, char *label) {
	Wsysmsg tx, rx;

	tx.type = Tlabel;
	tx.label = label;
	return displayrpc(d, &tx, &rx, NULL);
}

char *_displayrdsnarf(Display *d) {
	void   *p;
	char   *s;
	Wsysmsg tx, rx;

	tx.type = Trdsnarf;
	if (displayrpc(d, &tx, &rx, &p) < 0) {
		return NULL;
	}
	s = strdup(rx.snarf);
	free(p);
	return s;
}

int _displaywrsnarf(Display *d, char *snarf) {
	Wsysmsg tx, rx;

	tx.type = Twrsnarf;
	tx.snarf = snarf;
	return displayrpc(d, &tx, &rx, NULL);
}

int _displayrddraw(Display *d, void *v, int n) {
	void   *p;
	Wsysmsg tx, rx;

	tx.type = Trddraw;
	tx.count = n;
	if (displayrpc(d, &tx, &rx, &p) < 0) {
		return -1;
	}
	memmove(v, rx.data, rx.count);
	free(p);
	return rx.count;
}

int _displaywrdraw(Display *d, void *v, int n) {
	Wsysmsg tx, rx;

	tx.type = Twrdraw;
	tx.count = n;
	tx.data = v;
	if (displayrpc(d, &tx, &rx, NULL) < 0) {
		return -1;
	}
	return rx.count;
}

int _displaytop(Display *d) {
	Wsysmsg tx, rx;

	tx.type = Ttop;
	return displayrpc(d, &tx, &rx, NULL);
}

int _displayresize(Display *d, Rectangle r) {
	Wsysmsg tx, rx;

	tx.type = Tresize;
	tx.rect = r;
	return displayrpc(d, &tx, &rx, NULL);
}

static int canreadfd(int fd) {
	fd_set         rs, ws, xs;
	struct timeval tv;

	FD_ZERO(&rs);
	FD_ZERO(&ws);
	FD_ZERO(&xs);
	FD_SET(fd, &rs);
	FD_SET(fd, &xs);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	if (select(fd + 1, &rs, &ws, &xs, &tv) < 0) {
		return 0;
	}
	if (FD_ISSET(fd, &rs) || FD_ISSET(fd, &xs)) {
		return 1;
	}
	return 0;
}
