include ../config.mk

#	add -Iincludedir for any include directories that need to be searched
#	for posix header files (for UMIPS, add -I/usr/include/posix)
INCS=-I../include


#	add name of library orderer - use ":" if none exists
RANLIB=:

#	add name of library
AR=ar

CFLAGS+=-c $(INCS) $(STANDARDS) $(INCLUDES)

LIB=libmemdraw.a
CC?=c99

OBJ=alloc.o alloc-stub.o arc.o cload.o cload-stub.o cmap.o\
	cread.o draw.o draw-stub.o ellipse.o fillpoly.o hwdraw.o\
	iprint.o line.o load.o load-stub.o openmemsubfont.o poly.o\
	read.o string.o subfont.o unload.o unload-stub.o write.o


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

$(OBJ):	../include/u.h ../include/draw.h ../include/memdraw.h
