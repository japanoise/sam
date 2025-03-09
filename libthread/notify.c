/*
 * Signal handling for Plan 9 programs.
 * We stubbornly use the strings from Plan 9 instead
 * of the enumerated Unix constants.
 * There are some weird translations.  In particular,
 * a "kill" note is the same as SIGTERM in Unix.
 * There is no equivalent note to Unix's SIGKILL, since
 * it's not a deliverable signal anyway.
 *
 * We do not handle SIGABRT or SIGSEGV, mainly because
 * the thread library queues its notes for later, and we want
 * to dump core with the state at time of delivery.
 *
 * We have to add some extra entry points to provide the
 * ability to tweak which signals are deliverable and which
 * are acted upon.  Notifydisable and notifyenable play with
 * the process signal mask.  Notifyignore enables the signal
 * but will not call notifyf when it comes in.  This is occasionally
 * useful.
 */

#include <u.h>
#include <signal.h>
#include <thread.h>
#include <fmt.h>

int postnote(int who, int pid, char *msg) {
	int sig;

	sig = _p9strsig(msg);
	if (sig == 0) {
		werrstr("unknown note");
		return -1;
	}

	if (pid <= 0) {
		werrstr("bad pid in postnote");
		return -1;
	}

	switch (who) {
	default:
		werrstr("bad who in postnote");
		return -1;
	case PNPROC:
		return kill(pid, sig);
	case PNGROUP:
		if ((pid = getpgid(pid)) < 0) {
			return -1;
		}
		return killpg(pid, sig);
	}
}

#define NCONT 0 /* continue after note */
#define NDFLT 1 /* terminate after note */
#define NSAVE 2 /* clear note but hold state */
#define NRSTR 3 /* restore saved state */

#define NFN 33
static int (*onnot[NFN])(void *, char *);
static Lock onnotlock;

typedef struct Sig Sig;

struct Sig {
	int sig; /* signal number */
	int flags;
};

enum {
	Restart = 1 << 0,
	Ignore = 1 << 1,
	NoNotify = 1 << 2,
};

static Sig sigs[] = {
    SIGHUP,
    0,
    SIGINT,
    0,
    SIGQUIT,
    0,
    SIGILL,
    0,
    SIGTRAP,
    0,
/*	SIGABRT, 		0, 	*/
#ifdef SIGEMT
    SIGEMT,
    0,
#endif
    SIGFPE,
    0,
    SIGBUS,
    0,
    /*	SIGSEGV, 		0, 	*/
    SIGCHLD,
    Restart | Ignore,
    SIGSYS,
    0,
    SIGPIPE,
    Ignore,
    SIGALRM,
    0,
    SIGTERM,
    0,
    SIGTSTP,
    Restart | Ignore | NoNotify,
    /*	SIGTTIN,		Restart|Ignore, */
    /*	SIGTTOU,		Restart|Ignore, */
    SIGXCPU,
    0,
    SIGXFSZ,
    0,
    SIGVTALRM,
    0,
    SIGUSR1,
    0,
    SIGUSR2,
    0,
#ifdef SIGWINCH
    SIGWINCH,
    Restart | Ignore | NoNotify,
#endif
#ifdef SIGINFO
    SIGINFO,
    Restart | Ignore | NoNotify,
#endif
};

static Sig *findsig(int s) {
	int i;

	for (i = 0; i < nelem(sigs); i++) {
		if (sigs[i].sig == s) {
			return &sigs[i];
		}
	}
	return NULL;
}

/*
 * The thread library initializes _notejmpbuf to its own
 * routine which provides a per-pthread jump buffer.
 * If we're not using the thread library, we assume we are
 * single-threaded.
 */
typedef struct Jmp Jmp;

struct Jmp {
	p9jmp_buf b;
};

static Jmp onejmp;

static Jmp *getonejmp(void) { return &onejmp; }

Jmp *(*_notejmpbuf)(void) = getonejmp;
static void noteinit(void);

/*
 * Actual signal handler.
 */

static void (*notifyf)(void *, char *); /* Plan 9 handler */

static void signotify(int sig) {
	char tmp[64];
	Jmp *j;
	Sig *s;

	j = (*_notejmpbuf)();
	switch (sigsetjmp((void *)(j->b), 1)) {
	case 0:
		if (notifyf) {
			(*notifyf)(NULL, _p9sigstr(sig, tmp));
		}
		/* fall through */
	case 1: /* noted(NDFLT) */
		if (0) {
			print("DEFAULT %d\n", sig);
		}
		s = findsig(sig);
		if (s && (s->flags & Ignore)) {
			return;
		}
		signal(sig, SIG_DFL);
		raise(sig);
		_exit(1);
	case 2: /* noted(NCONT) */
		if (0) {
			print("HANDLED %d\n", sig);
		}
		return;
	}
}

static void signonotify(int sig) {}

int noted(int v) {
	siglongjmp((void *)((*_notejmpbuf)()->b), v == NCONT ? 2 : 1);
	abort();
	return 0;
}

int notify(void (*f)(void *, char *)) {
	static int init;

	notifyf = f;
	if (!init) {
		init = 1;
		noteinit();
	}
	return 0;
}

/*
 * Nonsense about enabling and disabling signals.
 */
typedef void Sighandler(int);

static Sighandler *handler(int s) {
	struct sigaction sa;

	sigaction(s, NULL, &sa);
	return sa.sa_handler;
}

static int notesetenable(int sig, int enabled) {
	sigset_t mask, omask;

	if (sig == 0) {
		return -1;
	}

	sigemptyset(&mask);
	sigaddset(&mask, sig);
	sigprocmask(enabled ? SIG_UNBLOCK : SIG_BLOCK, &mask, &omask);
	return !sigismember(&omask, sig);
}

int noteenable(char *msg) { return notesetenable(_p9strsig(msg), 1); }

int notedisable(char *msg) { return notesetenable(_p9strsig(msg), 0); }

static int notifyseton(int s, int on) {
	Sig             *sig;
	struct sigaction sa, osa;

	sig = findsig(s);
	if (sig == NULL) {
		return -1;
	}
	memset(&sa, 0, sizeof sa);
	sa.sa_handler = on ? signotify : signonotify;
	if (sig->flags & Restart) {
		sa.sa_flags |= SA_RESTART;
	}

	/*
	 * We can't allow signals within signals because there's
	 * only one jump buffer.
	 */
	sigfillset(&sa.sa_mask);

	/*
	 * Install handler.
	 */
	sigaction(sig->sig, &sa, &osa);
	return osa.sa_handler == signotify;
}

int notifyon(char *msg) { return notifyseton(_p9strsig(msg), 1); }

int notifyoff(char *msg) { return notifyseton(_p9strsig(msg), 0); }

/*
 * Initialization follows sigs table.
 */
static void noteinit(void) {
	int  i;
	Sig *sig;

	for (i = 0; i < nelem(sigs); i++) {
		sig = &sigs[i];
		/*
		 * If someone has already installed a handler,
		 * It's probably some ld preload nonsense,
		 * like pct (a SIGVTALRM-based profiler).
		 * Or maybe someone has already called notifyon/notifyoff.
		 * Leave it alone.
		 */
		if (handler(sig->sig) != SIG_DFL) {
			continue;
		}
		notifyseton(sig->sig, !(sig->flags & NoNotify));
	}
}

static void notifier(void *v, char *s) {
	int i;

	for (i = 0; i < NFN; i++) {
		if (onnot[i] && ((*onnot[i])(v, s))) {
			noted(NCONT);
			return;
		}
	}
	noted(NDFLT);
}

int atnotify(int (*f)(void *, char *), int in) {
	int        i, n, ret;
	static int init;

	if (!init) {
		notify(notifier);
		init = 1; /* assign = */
	}
	ret = 0;
	lock(&onnotlock);
	if (in) {
		for (i = 0; i < NFN; i++) {
			if (onnot[i] == 0) {
				onnot[i] = f;
				ret = 1;
				break;
			}
		}
	} else {
		n = 0;
		for (i = 0; i < NFN; i++) {
			if (onnot[i]) {
				if (ret == 0 && onnot[i] == f) {
					onnot[i] = 0;
					ret = 1;
				} else {
					n++;
				}
			}
		}
		if (n == 0) {
			init = 0;
			notify(0);
		}
	}
	unlock(&onnotlock);
	return ret;
}
