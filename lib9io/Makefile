include ../config.mk

#	add -Iincludedir for any include directories that need to be searched
#	for posix header files (for UMIPS, add -I/usr/include/posix)
INCS=-I../include


#	add name of library orderer - use ":" if none exists
RANLIB=:

#	add name of library
AR=ar

CFLAGS+=-c $(INCS) $(STANDARDS) $(INCLUDES)

LIB=lib9io.a
CC?=c99

OBJ=bflush.o binit.o bprint.o brdline.o brdstr.o bread.o bvprint.o\
	bwrite.o dofmt.o dorfmt.o errfmt.o errstr.o fltfmt.o fmt.o \
	fmtfd.o fmtfdflush.o fmtlocale.o fmtlock.o fmtprint.o\
	fmtquote.o fmtrune.o fmtstr.o fmtvprint.o fprint.o nan64.o\
	pow10.o print.o runesnprint.o runevsnprint.o seprint.o\
	smprint.o snprint.o sprint.o strtod.o\
	vfprint.o vseprint.o vsmprint.o vsnprint.o

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

$(OBJ):	../include/u.h ../include/bio.h ../include/fmt.h
