include ../config.mk

#	add -Iincludedir for any include directories that need to be searched
#	for posix header files (for UMIPS, add -I/usr/include/posix)
INCS=-I../include


#	add name of library orderer - use ":" if none exists
RANLIB=:

#	add name of library
AR=ar

CFLAGS+=-c $(INCS) $(STANDARDS) $(INCLUDES)

LIB=libmemlayer.a
CC?=c99

OBJ=draw.o lalloc.o layerop.o ldelete.o lhide.o line.o load.o\
	lorigin.o lsetrefresh.o ltofront.o ltorear.o unload.o


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

$(OBJ):	../include/u.h ../include/memlayer.h ../include/memdraw.h
