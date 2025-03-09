include ../config.mk

#	add -Iincludedir for any include directories that need to be searched
#	for posix header files (for UMIPS, add -I/usr/include/posix)
INCS=-I../include


#	add name of library orderer - use ":" if none exists
RANLIB=:

#	add name of library
AR=ar

CFLAGS+=-c $(INCS) $(STANDARDS) $(INCLUDES)

LIB=libdraw.a
CC?=c99

OBJ=allocimagemix.o alloc.o arith.o bezier.o border.o buildfont.o\
	bytesperline.o chan.o cloadimage.o computil.o creadimage.o\
	cursor.o debug.o drawclient.o drawfcall.o dial.o draw.o\
	drawrepl.o egetrect.o ellipse.o emenuhit.o event.o font.o\
	freesubfont.o getrect.o getsubfont.o icossin2.o icossin.o\
	init.o io.o iprint.o keyboard.o line.o loadimage.o menuhit.o\
	mkfont.o mouse.o mux.o openfont.o poly.o queue.o\
	readimage.o readsubfont.o rectclip.o replclipr.o rgb.o\
	scroll.o snarf.o stringbg.o string.o stringsubfont.o\
	stringwidth.o subfontcache.o subfontname.o subfont.o\
	thread.o unix.o unloadimage.o window.o writeimage.o\
	writesubfont.o wsys.o


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

$(OBJ):	../include/u.h ../include/draw.h ../include/drawfcall.h ../include/9event.h ../include/keyboard.h ../include/cursor.h ../include/bio.h ../include/mouse.h mux.h
