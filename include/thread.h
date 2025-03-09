#ifndef _THREAD_H_
#define _THREAD_H_ 1

#include <u.h>
#include "errstr.h"

/* AUTOLIB(thread) */

typedef struct Waitmsg {
	int   pid;     /* of loved one */
	ulong time[3]; /* of loved one & descendants */
	char *msg;
} Waitmsg;

typedef struct _Thread     _Thread;
typedef struct _Threadlist _Threadlist;

struct _Threadlist {
	_Thread *head;
	_Thread *tail;
};

extern _Thread *(*threadnow)(void);

/*
 *  synchronization
 */
typedef struct Lock Lock;

struct Lock {
	int             init;
	pthread_mutex_t mutex;
	int             held;
};

extern void lock(Lock *);
extern void unlock(Lock *);
extern int  canlock(Lock *);
extern int (*_lock)(Lock *, int, ulong);
extern void (*_unlock)(Lock *, ulong);

typedef struct QLock QLock;

struct QLock {
	Lock        l;
	_Thread    *owner;
	_Threadlist waiting;
};

extern void qlock(QLock *);
extern void qunlock(QLock *);
extern int  canqlock(QLock *);
extern int (*_qlock)(QLock *, int, ulong); /* do not use */
extern void (*_qunlock)(QLock *, ulong);

typedef struct Rendez Rendez;

struct Rendez {
	QLock      *l;
	_Threadlist waiting;
};

extern void rsleep(Rendez *); /* unlocks r->l, sleeps, locks r->l again */
extern int  rwakeup(Rendez *);
extern int  rwakeupall(Rendez *);
extern void (*_rsleep)(Rendez *, ulong); /* do not use */
extern int (*_rwakeup)(Rendez *, int, ulong);

typedef struct RWLock RWLock;

struct RWLock {
	Lock        l;
	int         readers;
	_Thread    *writer;
	_Threadlist rwaiting;
	_Threadlist wwaiting;
};

extern void rlock(RWLock *);
extern void runlock(RWLock *);
extern int  canrlock(RWLock *);
extern void wlock(RWLock *);
extern void wunlock(RWLock *);
extern int  canwlock(RWLock *);
extern int (*_rlock)(RWLock *, int, ulong); /* do not use */
extern int (*_wlock)(RWLock *, int, ulong);
extern void (*_runlock)(RWLock *, ulong);
extern void (*_wunlock)(RWLock *, ulong);

/*
 * basic procs and threads
 */
int      proccreate(void (*f)(void *arg), void *arg, uint stacksize);
int      threadcreate(void (*f)(void *arg), void *arg, uint stacksize);
void     threadexits(char *);
void     threadexitsall(char *);
void     threadsetname(char *, ...);
void     threadsetstate(char *, ...);
void     threadneedbackground(void);
char    *threadgetname(void);
int      threadyield(void);
int      threadidle(void);
void     _threadready(_Thread *);
void     _threadswitch(void);
void     _threadsetsysproc(void);
void     _threadsleep(Rendez *);
_Thread *_threadwakeup(Rendez *);
#define yield threadyield
int  threadid(void);
void _threadpin(void);
void _threadunpin(void);

/*
 * I am tired of making this mistake.
 */
#define exits do_not_call_exits_in_threaded_programs
#define _exits do_not_call__exits_in_threaded_programs

/*
 * signals
 */
void threadnotify(int (*f)(void *, char *), int);

/*
 * daemonize
 *
void	threaddaemonize(void);
 */

/*
 * per proc and thread data
 */
void **procdata(void);
void **threaddata(void);

/*
 * supplied by user instead of main.
 * mainstacksize is size of stack allocated to run threadmain
 */
void       threadmain(int argc, char *argv[]);
extern int mainstacksize;

/*
 * See needstack(3):
 * "Calling needstack indicates to the thread library that an external
 * routine is about to be called that will require n bytes of stack
 * space"
 */
void needstack(int n);

int threadmaybackground(void);

/*
 * channel communication
 */
typedef struct Alt       Alt;
typedef struct _Altarray _Altarray;
typedef struct Channel   Channel;

enum { CHANEND, CHANSND, CHANRCV, CHANNOP, CHANNOBLK };

struct Alt {
	Channel *c;
	void    *v;
	uint     op;
	_Thread *thread;
};

struct _Altarray {
	Alt **a;
	uint  n;
	uint  m;
};

struct Channel {
	uint      bufsize;
	uint      elemsize;
	uchar    *buf;
	uint      nbuf;
	uint      off;
	_Altarray asend;
	_Altarray arecv;
	char     *name;
};

/* [Edit .+1,./^$/ |cfn -h $PLAN9/src/libthread/channel.c] */
int      chanalt(Alt *alts);
Channel *chancreate(int elemsize, int elemcnt);
void     chanfree(Channel *c);
int      channbrecv(Channel *c, void *v);
void    *channbrecvp(Channel *c);
ulong    channbrecvul(Channel *c);
int      channbsend(Channel *c, void *v);
int      channbsendp(Channel *c, void *v);
int      channbsendul(Channel *c, ulong v);
int      chanrecv(Channel *c, void *v);
void    *chanrecvp(Channel *c);
ulong    chanrecvul(Channel *c);
int      chansend(Channel *c, void *v);
int      chansendp(Channel *c, void *v);
int      chansendul(Channel *c, ulong v);
void     chansetname(Channel *c, char *fmt, ...);

#define alt chanalt
#define nbrecv channbrecv
#define nbrecvp channbrecvp
#define nbrecvul channbrecvul
#define nbsend channbsend
#define nbsendp channbsendp
#define nbsendul channbsendul
#define recv chanrecv
#define recvp chanrecvp
#define recvul chanrecvul
#define send chansend
#define sendp chansendp
#define sendul chansendul

/*
 * reference counts
 */
typedef struct Ref Ref;

struct Ref {
	Lock lock;
	long ref;
};

long decref(Ref *r);
long incref(Ref *r);

/*
 * slave i/o processes
 */
typedef struct Ioproc Ioproc;

/* [Edit .+1,/^$/ |cfn -h $PLAN9/src/libthread/io*.c] */
void    closeioproc(Ioproc *io);
long    iocall(Ioproc *io, long (*op)(va_list *), ...);
int     ioclose(Ioproc *io, int fd);
int     iodial(Ioproc *io, char *addr, char *local, char *dir, int *cdfp);
void    iointerrupt(Ioproc *io);
int     ioopen(Ioproc *io, char *path, int mode);
Ioproc *ioproc(void);
long    ioread(Ioproc *io, int fd, void *a, long n);
int     ioread9pmsg(Ioproc *, int, void *, int);
long    ioreadn(Ioproc *io, int fd, void *a, long n);
int     iorecvfd(Ioproc *, int);
int     iosendfd(Ioproc *, int, int);
int     iosleep(Ioproc *io, long n);
long    iowrite(Ioproc *io, int fd, void *a, long n);

/*
 * exec external programs
 */
void     threadexec(Channel *, int[3], char *, char *[]);
void     threadexecl(Channel *, int[3], char *, ...);
int      threadspawn(int[3], char *, char *[]);
int      threadspawnd(int[3], char *, char *[], char *);
int      threadspawnl(int[3], char *, ...);
Channel *threadwaitchan(void);

/*
 * alternate interface to threadwaitchan - don't use both!
 */
Waitmsg *procwait(int pid);

#ifdef RFMEM /* FreeBSD, OpenBSD */
#undef RFFDG
#undef RFNOTEG
#undef RFPROC
#undef RFMEM
#undef RFNOWAIT
#undef RFCFDG
#undef RFNAMEG
#undef RFENVG
#undef RFCENVG
#undef RFCFDG
#undef RFCNAMEG
#endif

enum {
	RFNAMEG = (1 << 0),
	RFENVG = (1 << 1),
	RFFDG = (1 << 2),
	RFNOTEG = (1 << 3),
	RFPROC = (1 << 4),
	RFMEM = (1 << 5),
	RFNOWAIT = (1 << 6),
	RFCNAMEG = (1 << 10),
	RFCENVG = (1 << 11),
	RFCFDG = (1 << 12)
	/*      RFREND          = (1<<13), */
	/*      RFNOMNT         = (1<<14) */
};

int p9rfork(int flags);

#endif /* _THREADH_ */
