VERSION=1.1

CC=gcc
CFLAGS=-Wall -Wno-pointer-sign -g
INSTALL=install

# these locations seem to work for SuSE and Fedora
# prefix = $(HOME)
prefix = $(DESTDIR)/usr
BINDIR = $(prefix)/bin
DATADIR = $(prefix)/share
DESKTOPDIR = $(DATADIR)/applications
ICONPATH = $(DATADIR)/icons/hicolor
ICONDIR = $(ICONPATH)/scalable/apps
MANDIR = $(DATADIR)/man/man1
gtk_update_icon_cache = gtk-update-icon-cache -f -t $(ICONPATH)

NAME = subsurface
ICONFILE = $(NAME).svg
DESKTOPFILE = $(NAME).desktop
MANFILES = $(NAME).1

# find libdivecomputer; we don't trust pkg-config here given how young
# libdivecomputer still is - so we check /usr/local and /usr and then we
# give up. You can override by simply setting it here
#
libdc-local := $(wildcard /usr/local/lib/libdivecomputer.a)
libdc-local64 := $(wildcard /usr/local/lib64/libdivecomputer.a)
libdc-usr := $(wildcard /usr/lib/libdivecomputer.a)
libdc-usr64 := $(wildcard /usr/lib64/libdivecomputer.a)

ifneq ($(strip $(libdc-local)),)
	LIBDIVECOMPUTERDIR = /usr/local
	LIBDIVECOMPUTERINCLUDES = $(LIBDIVECOMPUTERDIR)/include/libdivecomputer
	LIBDIVECOMPUTERARCHIVE = $(LIBDIVECOMPUTERDIR)/lib/libdivecomputer.a
else ifneq ($(strip $(libdc-local64)),)
	LIBDIVECOMPUTERDIR = /usr/local
	LIBDIVECOMPUTERINCLUDES = $(LIBDIVECOMPUTERDIR)/include/libdivecomputer
	LIBDIVECOMPUTERARCHIVE = $(LIBDIVECOMPUTERDIR)/lib64/libdivecomputer.a
else ifneq ($(strip $(libdc-usr)),)
	LIBDIVECOMPUTERDIR = /usr
	LIBDIVECOMPUTERINCLUDES = $(LIBDIVECOMPUTERDIR)/include/libdivecomputer
	LIBDIVECOMPUTERARCHIVE = $(LIBDIVECOMPUTERDIR)/lib/libdivecomputer.a
else ifneq ($(strip $(libdc-usr64)),)
	LIBDIVECOMPUTERDIR = /usr
	LIBDIVECOMPUTERINCLUDES = $(LIBDIVECOMPUTERDIR)/include/libdivecomputer
	LIBDIVECOMPUTERARCHIVE = $(LIBDIVECOMPUTERDIR)/lib64/libdivecomputer.a
else
$(error Cannot find libdivecomputer - please edit Makefile)
endif

# Libusb-1.0 is only required if libdivecomputer was built with it.
# And libdivecomputer is only built with it if libusb-1.0 is
# installed. So get libusb if it exists, but don't complain
# about it if it doesn't.
LIBUSB = $(shell pkg-config --libs libusb-1.0 2> /dev/null)

# it appears that xml2-config isn't included in the libxml2 package for
# MinGW - so under Windows you may want to replace this with a hardcoded
# path to the installdir - something like
# LIBXML2 = -L/c/opt/gtk/lib -lxml2
LIBXML2 = $(shell xml2-config --libs)
LIBGTK = $(shell pkg-config --libs gtk+-2.0 glib-2.0 gconf-2.0)
LIBDIVECOMPUTERCFLAGS = -I$(LIBDIVECOMPUTERINCLUDES)
LIBDIVECOMPUTER = $(LIBDIVECOMPUTERARCHIVE) $(LIBUSB)

LIBS = $(LIBXML2) $(LIBGTK) $(LIBDIVECOMPUTER) -lpthread

OBJS =	main.o dive.o profile.o info.o equipment.o divelist.o \
	parse-xml.o save-xml.o libdivecomputer.o print.o uemis.o \
	gtk-gui.o

$(NAME): $(OBJS)
	$(CC) $(LDFLAGS) -o $(NAME) $(OBJS) $(LIBS)

install: $(NAME)
	$(INSTALL) -d -m 755 $(BINDIR)
	$(INSTALL) $(NAME) $(BINDIR)
	$(INSTALL) -d -m 755 $(DESKTOPDIR)
	$(INSTALL) $(DESKTOPFILE) $(DESKTOPDIR)
	$(INSTALL) -d -m 755 $(ICONDIR)
	$(INSTALL) $(ICONFILE) $(ICONDIR)
	$(gtk_update_icon_cache)
	$(INSTALL) -d -m 755 $(MANDIR)
	$(INSTALL) -m 644 $(MANFILES) $(MANDIR)

# it appears that xml2-config isn't included in the libxml2 package for
# MinGW - so under Windows you may want to replace this with a hardcoded
# path to the inclde dir - something like
#
# XML2INCLUDE = -I/c/opt/gtk/include/libxml2
#
# parse-xml.o: parse-xml.c dive.h
#	$(CC) $(CFLAGS) `pkg-config --cflags glib-2.0` -c $(XML2INCLUDE)  parse-xml.c

 
parse-xml.o: parse-xml.c dive.h
	$(CC) $(CFLAGS) `pkg-config --cflags glib-2.0` -c `xml2-config --cflags`  parse-xml.c

save-xml.o: save-xml.c dive.h
	$(CC) $(CFLAGS) `pkg-config --cflags glib-2.0` -c save-xml.c

dive.o: dive.c dive.h
	$(CC) $(CFLAGS) `pkg-config --cflags glib-2.0` -c dive.c

main.o: main.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) `pkg-config --cflags glib-2.0 gconf-2.0` \
		-c main.c

profile.o: profile.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0 glib-2.0` -c profile.c

info.o: info.c dive.h display.h display-gtk.h divelist.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0 glib-2.0` -c info.c

equipment.o: equipment.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0 glib-2.0` -c equipment.c

divelist.o: divelist.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0 glib-2.0` -c divelist.c

print.o: print.c dive.h display.h display-gtk.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0 glib-2.0` -c print.c

libdivecomputer.o: libdivecomputer.c dive.h display.h display-gtk.h libdivecomputer.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0 glib-2.0` \
			$(LIBDIVECOMPUTERCFLAGS) \
			-c libdivecomputer.c

gtk-gui.o: gtk-gui.c dive.h display.h divelist.h display-gtk.h libdivecomputer.h Makefile
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0 glib-2.0 gconf-2.0` \
			$(LIBDIVECOMPUTERCFLAGS) \
			-DVERSION_STRING='"v$(VERSION)"' \
			-c gtk-gui.c

uemis.o: uemis.c uemis.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0 glib-2.0` -c uemis.c

clean:
	rm -f $(OBJS) *~ $(NAME)
