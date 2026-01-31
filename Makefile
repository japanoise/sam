#	Copyright (c) 1998 Lucent Technologies - All rights reserved.
#	Changes Copyright (c) 2014-2015 Rob King
#
#	master makefile for sam.  configure sub-makefiles first.
#

MODE?=user

all:    config.mk lXg lframe lutf samdir samtermdir docdir

# Note: config.mk is not included in this makefile. That is by design.
config.mk:
	cp config.mk.def config.mk

lutf:
	cd libutf; $(MAKE)

lXg:    lutf
	cd libXg; $(MAKE)

lframe: lutf
	cd libframe; $(MAKE)

docdir:
	cd doc; $(MAKE)

samdir: lutf lXg lframe
	cd sam; $(MAKE)

samtermdir: lutf lXg lframe samdir
	cd samterm; $(MAKE)

install:
	cd libXg; $(MAKE) install
	cd libframe; $(MAKE) install
	cd sam; $(MAKE) install
	cd samterm; $(MAKE) install
	cd doc; $(MAKE) install
	cd ssam; $(MAKE) install

uninstall:
	cd libXg; $(MAKE) uninstall
	cd libframe; $(MAKE) uninstall
	cd sam; $(MAKE) uninstall
	cd samterm; $(MAKE) uninstall
	cd doc; $(MAKE) uninstall
	cd ssam; $(MAKE) uninstall

clean:
	cd libXg; $(MAKE) clean
	cd libframe; $(MAKE) clean
	cd sam; $(MAKE) clean
	cd samterm; $(MAKE) clean
	cd ssam; $(MAKE) clean
	cd libutf; $(MAKE) clean

format:
	find ./ -iname "*.[ch]" | xargs clang-format -i

nuke: clean
	rm -f config.mk
