#include <u.h>
#include <fmt.h>
#include <signal.h>
#include <sys/types.h>
#define __USE_MISC
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <thread.h>
#include "threadimpl.h"

static char qsep[] = " \t\r\n";

static char *qtoken(char *s, char *sep) {
	int   quoting;
	char *t;

	quoting = 0;
	t = s; /* s is output string, t is input string */
	while (*t != '\0' && (quoting || utfrune(sep, *t) == NULL)) {
		if (*t != '\'') {
			*s++ = *t++;
			continue;
		}
		/* *t is a quote */
		if (!quoting) {
			quoting = 1;
			t++;
			continue;
		}
		/* quoting and we're on a quote */
		if (t[1] != '\'') {
			/* end of quoted section; absorb closing quote */
			t++;
			quoting = 0;
			continue;
		}
		/* doubled quote; fold one quote into two */
		t++;
		*s++ = *t++;
	}
	if (*s != '\0') {
		*s = '\0';
		if (t == s) {
			t++;
		}
	}
	return t;
}

static char *etoken(char *t, char *sep) {
	int quoting;

	/* move to end of next token */
	quoting = 0;
	while (*t != '\0' && (quoting || utfrune(sep, *t) == NULL)) {
		if (*t != '\'') {
			t++;
			continue;
		}
		/* *t is a quote */
		if (!quoting) {
			quoting = 1;
			t++;
			continue;
		}
		/* quoting and we're on a quote */
		if (t[1] != '\'') {
			/* end of quoted section; absorb closing quote */
			t++;
			quoting = 0;
			continue;
		}
		/* doubled quote; fold one quote into two */
		t += 2;
	}
	return t;
}

int gettokens(char *s, char **args, int maxargs, char *sep) {
	int nargs;

	for (nargs = 0; nargs < maxargs; nargs++) {
		while (*s != '\0' && utfrune(sep, *s) != NULL) {
			*s++ = '\0';
		}
		if (*s == '\0') {
			break;
		}
		args[nargs] = s;
		s = etoken(s, sep);
	}

	return nargs;
}

int tokenize(char *s, char **args, int maxargs) {
	int nargs;

	for (nargs = 0; nargs < maxargs; nargs++) {
		while (*s != '\0' && utfrune(qsep, *s) != NULL) {
			s++;
		}
		if (*s == '\0') {
			break;
		}
		args[nargs] = s;
		s = qtoken(s, qsep);
	}

	return nargs;
}

static Lock     thewaitlock;
static Channel *thewaitchan;

#ifndef WCOREDUMP /* not on Mac OS X Tiger */
#define WCOREDUMP(status) 0
#endif

static struct {
	int   sig;
	char *str;
} tab[] = {
    SIGHUP,    "hangup",
    SIGINT,    "interrupt",
    SIGQUIT,   "quit",
    SIGILL,    "sys: illegal instruction",
    SIGTRAP,   "sys: breakpoint",
    SIGABRT,   "sys: abort",
#ifdef SIGEMT
    SIGEMT,    "sys: emulate instruction executed",
#endif
    SIGFPE,    "sys: fp: trap",
    SIGKILL,   "sys: kill",
    SIGBUS,    "sys: bus error",
    SIGSEGV,   "sys: segmentation violation",
    SIGALRM,   "alarm",
    SIGTERM,   "kill",
    SIGURG,    "sys: urgent condition on socket",
    SIGSTOP,   "sys: stop",
    SIGTSTP,   "sys: tstp",
    SIGCONT,   "sys: cont",
    SIGCHLD,   "sys: child",
    SIGTTIN,   "sys: ttin",
    SIGTTOU,   "sys: ttou",
#ifdef SIGIO /* not on Mac OS X Tiger */
    SIGIO,     "sys: i/o possible on fd",
#endif
    SIGXCPU,   "sys: cpu time limit exceeded",
    SIGXFSZ,   "sys: file size limit exceeded",
    SIGVTALRM, "sys: virtual time alarm",
    SIGPROF,   "sys: profiling timer alarm",
#ifdef SIGWINCH /* not on Mac OS X Tiger */
    SIGWINCH,  "sys: window size change",
#endif
#ifdef SIGINFO
    SIGINFO,   "sys: status request",
#endif
    SIGUSR1,   "sys: usr1",
    SIGUSR2,   "sys: usr2",
    SIGPIPE,   "sys: write on closed pipe",
};

char *_p9sigstr(int sig, char *tmp) {
	int i;

	for (i = 0; i < nelem(tab); i++) {
		if (tab[i].sig == sig) {
			return tab[i].str;
		}
	}
	if (tmp == NULL) {
		return NULL;
	}
	sprint(tmp, "sys: signal %d", sig);
	return tmp;
}

int _p9strsig(char *s) {
	int i;

	for (i = 0; i < nelem(tab); i++) {
		if (strcmp(s, tab[i].str) == 0) {
			return tab[i].sig;
		}
	}
	return 0;
}

static int _await(int pid4, char *str, int n, int opt) {
	int           pid, status, cd;
	struct rusage ru;
	char          buf[128], tmp[64];
	ulong         u, s;

	for (;;) {
		/* On Linux, pid==-1 means anyone; on SunOS, it's pid==0. */
		if (pid4 == -1) {
			pid = wait3(&status, opt, &ru);
		} else {
			pid = wait4(pid4, &status, opt, &ru);
		}
		if (pid <= 0) {
			return -1;
		}
		u = ru.ru_utime.tv_sec * 1000 +
		    ((ru.ru_utime.tv_usec + 500) / 1000);
		s = ru.ru_stime.tv_sec * 1000 +
		    ((ru.ru_stime.tv_usec + 500) / 1000);
		if (WIFEXITED(status)) {
			status = WEXITSTATUS(status);
			if (status) {
				snprint(buf, sizeof buf, "%d %lud %lud %lud %d",
					pid, u, s, u + s, status);
			} else {
				snprint(buf, sizeof buf, "%d %lud %lud %lud ''",
					pid, u, s, u + s, status);
			}
			strecpy(str, str + n, buf);
			return strlen(str);
		}
		if (WIFSIGNALED(status)) {
			cd = WCOREDUMP(status);
			snprint(buf, sizeof buf,
				"%d %lud %lud %lud 'signal: %s%s'", pid, u, s,
				u + s, _p9sigstr(WTERMSIG(status), tmp),
				cd ? " (core dumped)" : "");
			strecpy(str, str + n, buf);
			return strlen(str);
		}
	}
}

int await(char *str, int n) { return _await(-1, str, n, 0); }

int awaitnohang(char *str, int n) { return _await(-1, str, n, WNOHANG); }

int awaitfor(int pid, char *str, int n) { return _await(pid, str, n, 0); }

static Waitmsg *_wait(int n, char *buf) {
	int      l;
	char    *fld[5];
	Waitmsg *w;

	if (n <= 0) {
		return NULL;
	}
	buf[n] = '\0';
	if (tokenize(buf, fld, nelem(fld)) != nelem(fld)) {
		werrstr("couldn't parse wait message");
		return NULL;
	}
	l = strlen(fld[4]) + 1;
	w = malloc(sizeof(Waitmsg) + l);
	if (w == NULL) {
		return NULL;
	}
	w->pid = atoi(fld[0]);
	w->time[0] = atoi(fld[1]);
	w->time[1] = atoi(fld[2]);
	w->time[2] = atoi(fld[3]);
	w->msg = (char *)&w[1];
	memmove(w->msg, fld[4], l);
	return w;
}

Waitmsg *p9wait(void) {
	char buf[256];

	return _wait(await(buf, sizeof buf - 1), buf);
}

Waitmsg *waitnohang(void) {
	char buf[256];

	return _wait(awaitnohang(buf, sizeof buf - 1), buf);
}

Waitmsg *waitfor(int pid) {
	char buf[256];

	return _wait(awaitfor(pid, buf, sizeof buf - 1), buf);
}

static void execproc(void *v) {
	int      pid;
	Channel *c;
	Execjob *e;
	Waitmsg *w;

	e = v;
	pid = _threadspawn(e->fd, e->cmd, e->argv, e->dir);
	sendul(e->c, pid);
	if (pid > 0) {
		w = waitfor(pid);
		if ((c = thewaitchan) != NULL) {
			sendp(c, w);
		} else {
			free(w);
		}
	}
	threadexits(NULL);
}

int _runthreadspawn(int *fd, char *cmd, char **argv, char *dir) {
	int     pid;
	Execjob e;

	e.fd = fd;
	e.cmd = cmd;
	e.argv = argv;
	e.dir = dir;
	e.c = chancreate(sizeof(void *), 0);
	proccreate(execproc, &e, 65536);
	pid = recvul(e.c);
	chanfree(e.c);
	return pid;
}

Channel *threadwaitchan(void) {
	if (thewaitchan) {
		return thewaitchan;
	}
	lock(&thewaitlock);
	if (thewaitchan) {
		unlock(&thewaitlock);
		return thewaitchan;
	}
	thewaitchan = chancreate(sizeof(Waitmsg *), 4);
	chansetname(thewaitchan, "threadwaitchan");
	unlock(&thewaitlock);
	return thewaitchan;
}

int _threadspawn(int fd[3], char *cmd, char *argv[], char *dir) {
	int  i, n, p[2], pid;
	char exitstr[100];

	notifyoff("sys: child"); /* do not let child note kill us */
	if (pipe(p) < 0) {
		return -1;
	}
	if (fcntl(p[0], F_SETFD, 1) < 0 || fcntl(p[1], F_SETFD, 1) < 0) {
		close(p[0]);
		close(p[1]);
		return -1;
	}
	switch (pid = fork()) {
	case -1:
		close(p[0]);
		close(p[1]);
		return -1;
	case 0:
		/* can't RFNOTEG - will lose tty */
		if (dir != NULL) {
			chdir(dir); /* best effort */
		}
		dup2(fd[0], 0);
		dup2(fd[1], 1);
		dup2(fd[2], 2);
		if (!isatty(0) && !isatty(1) && !isatty(2)) {
			p9rfork(RFNOTEG);
		}
		for (i = 3; i < 100; i++) {
			if (i != p[1]) {
				close(i);
			}
		}
		execvp(cmd, argv);
		fprint(p[1], "%d", errno);
		close(p[1]);
		_exit(0);
	}

	close(p[1]);
	n = read(p[0], exitstr, sizeof exitstr - 1);
	close(p[0]);
	if (n > 0) { /* exec failed */
		free(waitfor(pid));
		exitstr[n] = 0;
		errno = atoi(exitstr);
		return -1;
	}

	close(fd[0]);
	if (fd[1] != fd[0]) {
		close(fd[1]);
	}
	if (fd[2] != fd[1] && fd[2] != fd[0]) {
		close(fd[2]);
	}
	return pid;
}

int threadspawn(int fd[3], char *cmd, char *argv[]) {
	return _runthreadspawn(fd, cmd, argv, NULL);
}

int threadspawnd(int fd[3], char *cmd, char *argv[], char *dir) {
	return _runthreadspawn(fd, cmd, argv, dir);
}

int threadspawnl(int fd[3], char *cmd, ...) {
	char  **argv, *s;
	int     n, pid;
	va_list arg;

	va_start(arg, cmd);
	for (n = 0; va_arg(arg, char *) != NULL; n++)
		;
	n++;
	va_end(arg);

	argv = malloc(n * sizeof(argv[0]));
	if (argv == NULL) {
		return -1;
	}

	va_start(arg, cmd);
	for (n = 0; (s = va_arg(arg, char *)) != NULL; n++) {
		argv[n] = s;
	}
	argv[n] = 0;
	va_end(arg);

	pid = threadspawn(fd, cmd, argv);
	free(argv);
	return pid;
}

int _threadexec(Channel *cpid, int fd[3], char *cmd, char *argv[]) {
	int pid;

	pid = threadspawn(fd, cmd, argv);
	if (cpid) {
		if (pid < 0) {
			chansendul(cpid, ~0);
		} else {
			chansendul(cpid, pid);
		}
	}
	return pid;
}

void threadexec(Channel *cpid, int fd[3], char *cmd, char *argv[]) {
	if (_threadexec(cpid, fd, cmd, argv) >= 0) {
		threadexits("threadexec");
	}
}

void threadexecl(Channel *cpid, int fd[3], char *cmd, ...) {
	char  **argv, *s;
	int     n, pid;
	va_list arg;

	va_start(arg, cmd);
	for (n = 0; va_arg(arg, char *) != NULL; n++)
		;
	n++;
	va_end(arg);

	argv = malloc(n * sizeof(argv[0]));
	if (argv == NULL) {
		if (cpid) {
			chansendul(cpid, ~0);
		}
		return;
	}

	va_start(arg, cmd);
	for (n = 0; (s = va_arg(arg, char *)) != NULL; n++) {
		argv[n] = s;
	}
	argv[n] = 0;
	va_end(arg);

	pid = _threadexec(cpid, fd, cmd, argv);
	free(argv);

	if (pid >= 0) {
		threadexits("threadexecl");
	}
}
