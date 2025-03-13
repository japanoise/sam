include ../config.mk

#	add -Iincludedir for any include directories that need to be searched
#	for posix header files (for UMIPS, add -I/usr/include/posix)
INCS=-I../include

CFLAGS+=$(STANDARDS) $(INCS) $(INCLUDES) -DRXPATH='"$(RXPATH)"' \
	$(shell ./mkwsysrules.sh cflags)
SAMDIR=$(BINDIR)

LIBS=../lib9io/lib9io.a ../libdraw/libdraw.a\
	../libthread/libthread.a ../libmemdraw/libmemdraw.a\
	../libmemlayer/libmemlayer.a ../libutf/libutf.a
CC?=c99

OBJ=devdraw.o latin1.o mouseswap.o srv.o winsize.o \
	$(shell ./mkwsysrules.sh objects)

debug: CFLAGS+=-g -O0 -DDEBUG
debug: devdraw

all: devdraw

devdraw: $(OBJ) $(LIBS)
	$(CC) -o $@ $(OBJ) $(LIBS) $(LDFLAGS)

latin1.o: latin1.h

latin1.h: keyboard o.mklatinkbd
	./o.mklatinkbd -r keyboard | sed 's/, }/ }/' > $@

o.mklatinkbd: mklatinkbd.o $(LIBS)
	$(CC) -g -O0 -o $@ $< $(LIBS)

clean:
	rm -f *.o core devdraw latin1.h o.*

nuke: clean
	rm -f devdraw

install: devdraw
	mkdir -p $(SAMDIR)
	cp devdraw $(SAMDIR)/devdraw

uninstall:
	rm -f $(SAMDIR)/devdraw

$(OBJ): $(wildcard ../include/*.h) $(shell ./mkwsysrules.sh headers)
