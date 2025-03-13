#	Copyright (c) 1998 Lucent Technologies - All rights reserved.
#	Changes Copyright (c) 2014-2015 Rob King
#
#	master makefile for sam.  configure sub-makefiles first.
#

MODE?=user

all:    config.mk lXg lframe lutf samdir samtermdir docdir

config.mk:
	cp config.mk.def config.mk

lutf:
	cd libutf; $(MAKE)

lXg:
	cd libXg; $(MAKE)

lframe:
	cd libframe; $(MAKE)

lthread:
	cd libthread; $(MAKE)

l9io:
	cd lib9io; $(MAKE)

ldraw:
	cd libdraw; $(MAKE)

lmemdraw:
	cd libmemdraw; $(MAKE)

lmemlayer:
	cd libmemlayer; $(MAKE)

devdrawclient: lthread l9io ldraw lmemdraw lmemlayer lutf
	cd devdraw; $(MAKE)

docdir:
	cd doc; $(MAKE)

samdir: lutf lXg lframe
	cd sam; $(MAKE)

samtermdir: lutf lXg lframe samdir
	cd samterm; $(MAKE)

install:
	@xdg-desktop-menu install --mode $(MODE) deadpixi-sam.desktop || echo "unable to install desktop entry"
	cd devdraw; $(MAKE) install
	cd libXg; $(MAKE) install
	cd libframe; $(MAKE) install
	cd sam; $(MAKE) install
	cd samterm; $(MAKE) install
	cd doc; $(MAKE) install
	cd ssam; $(MAKE) install

uninstall:
	@xdg-desktop-menu uninstall --mode $(MODE) deadpixi-sam.desktop || echo "unable to uninstall desktop entry"
	cd devdraw; $(MAKE) uninstall
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
	cd libthread; $(MAKE) clean
	cd lib9io; $(MAKE) clean
	cd libdraw; $(MAKE) clean
	cd libmemdraw; $(MAKE) clean
	cd libmemlayer; $(MAKE) clean
	cd devdraw; $(MAKE) clean

format:
	find ./ -iname "*.[ch]" | xargs clang-format -i

nuke: clean
	rm -f config.mk
