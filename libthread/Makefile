include ../config.mk

#	add -Iincludedir for any include directories that need to be searched
#	for posix header files (for UMIPS, add -I/usr/include/posix)
INCS=-I../include


#	add name of library orderer - use ":" if none exists
RANLIB=:

#	add name of library
AR=ar

CFLAGS+=-c $(INCS) $(STANDARDS) $(INCLUDES)

LIB=libthread.a
CC?=c99

OBJ= bg.o channel.o daemonize.o exec.o ioproc.o lock.o\
	notify.o pthread.o qlock.o ref.o rfork.o thread.o wait.o

all:	$(LIB)

$(LIB):	$(OBJ)
	$(AR) rv $(LIB) $(OBJ)
	$(RANLIB) $(LIB)

clean:
	rm -f *.o *.a

nuke:	clean
	rm -f $(LIB)

install:	$(LIB)

uninstall:

$(OBJ):	../include/u.h threadimpl.h ../include/thread.h

# Extra stuff in p9p's mkfile, probably not needed - doesn't seem to
# be any asm files in the master branch over there afaict.

# tprimes: test/tprimes.$O
#	9l -o $target test/$target.$O
# tspawn: test/tspawn.$O
#	9l -o $target test/$target.$O
# tspawnloop: test/tspawnloop.$O
#	9l -o $target test/$target.$O
#
# tprimes tspawn tspawnloop: $PLAN9/lib/$LIB
#
# # cannot use generic .S rule because it conflicts
# # with generic .s rule in mkcommon on case-insensitive
# # systems like Mac OS X.
#
# OpenBSD-%-asm.$O:	OpenBSD-%-asm.S
#	$CC $CFLAGS OpenBSD-$stem-asm.S
#
# Linux-sparc64-context.$O: Linux-sparc64-context.S
#	$CC -m64 -mcpu=v9 $CFLAGS Linux-sparc64-context.S
#
# sparc64-ucontext.$O: sparc64-ucontext.c
#	$CC -m64 -mcpu=v9 $CFLAGS sparc64-ucontext.c
#
# test:V: tprimes tspawn
#	primes 1 10007 >p1.txt
#	$PLAN9/bin/time ./tprimes 10000 >tp1.txt
#	cmp p1.txt tp1.txt
#	primes 1 1009 >p2.txt
#	$PLAN9/bin/time ./tprimes 1000 >tp2.txt
#	cmp p2.txt tp2.txt
#	echo tspawn should take 3 seconds, not 6
#	$PLAN9/bin/time ./tspawn sleep 3 >/dev/null
#
# CLEANFILES=p1.txt p2.txt tp1.txt tp2.txt test/*.$O tprimes tspawn
