# Copyright 2014-2014 Rob King <jking@deadpixi.com>

include ../config.mk

all:

DESKTOPFILE=deadpixi-sam.desktop

install-desktop:
	mkdir -p $(DESTDIR)/share/applications/
	cp $(DESKTOPFILE) $(DESTDIR)/share/applications/

uninstall-desktop:
	rm $(DESTDIR)/share/applications/$(DESKTOPFILE)

install: sam.1 install-desktop
	mkdir -p "$(MANDIR)/man1"
	cp sam.1 "$(MANDIR)/man1"
	ln -sf "$(MANDIR)/man1/sam.1" "$(MANDIR)/man1/B.1"
	ln -sf "$(MANDIR)/man1/sam.1" "$(MANDIR)/man1/samterm.1"
	ln -sf "$(MANDIR)/man1/sam.1" "$(MANDIR)/man1/rsam.1"
	ln -sf "$(MANDIR)/man1/sam.1" "$(MANDIR)/man1/sam.save.1"
	cp ssam.1 "$(MANDIR)/man1"
	mkdir -p "$(MANDIR)/man5"
	cp samrc.5 "$(MANDIR)/man5"

uninstall: uninstall-desktop
	rm -f "$(MANDIR)/man1/ssam.1"
	rm -f "$(MANDIR)/man1/sam.1"
	rm -f "$(MANDIR)/man1/B.1"
	rm -f "$(MANDIR)/man1/samterm.1"
	rm -f "$(MANDIR)/man1/rsam.1"
	rm -f "$(MANDIR)/man1/sam.save.1"
	rm -f "$(MANDIR)/man5/samrc.5"
