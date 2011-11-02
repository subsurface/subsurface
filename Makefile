VERSION=1.1

CC=gcc
CFLAGS=-Wall -Wno-pointer-sign -g
INSTALL=install
PKGCONFIG=pkg-config
XML2CONFIG=xml2-config

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

MACOSXINSTALL = /Applications/Subsurface.app
MACOSXFILES = packaging/macosx

# find libdivecomputer
# First deal with the cross compile environment.
# For the native case, Linus doesn't want to trust pkg-config given
# how young libdivecomputer still is - so we check the typical
# subdirectories of /usr/local and /usr and then we give up. You can
# override by simply setting it here
#
ifeq ($(CC), i686-w64-mingw32-gcc)
# ok, we are cross building for Windows
	LIBDIVECOMPUTERDIR = /usr/i686-w64-mingw32/sys-root/mingw/include/libdivecomputer
	LIBDIVECOMPUTERINCLUDES = `$(PKGCONFIG) --cflags libdivecomputer`
	LIBDIVECOMPUTERARCHIVE = `$(PKGCONFIG) --libs libdivecomputer`
	RESFILE = packaging/windows/subsurface.res
	LDFLAGS += -Wl,-subsystem,windows
else

libdc-local := $(wildcard /usr/local/lib/libdivecomputer.a)
libdc-local64 := $(wildcard /usr/local/lib64/libdivecomputer.a)
libdc-usr := $(wildcard /usr/lib/libdivecomputer.a)
libdc-usr64 := $(wildcard /usr/lib64/libdivecomputer.a)

ifneq ($(strip $(libdc-local)),)
	LIBDIVECOMPUTERDIR = /usr/local
	LIBDIVECOMPUTERINCLUDES = -I$(LIBDIVECOMPUTERDIR)/include/libdivecomputer
	LIBDIVECOMPUTERARCHIVE = $(LIBDIVECOMPUTERDIR)/lib/libdivecomputer.a
else ifneq ($(strip $(libdc-local64)),)
	LIBDIVECOMPUTERDIR = /usr/local
	LIBDIVECOMPUTERINCLUDES = -I$(LIBDIVECOMPUTERDIR)/include/libdivecomputer
	LIBDIVECOMPUTERARCHIVE = $(LIBDIVECOMPUTERDIR)/lib64/libdivecomputer.a
else ifneq ($(strip $(libdc-usr)),)
	LIBDIVECOMPUTERDIR = /usr
	LIBDIVECOMPUTERINCLUDES = -I$(LIBDIVECOMPUTERDIR)/include/libdivecomputer
	LIBDIVECOMPUTERARCHIVE = $(LIBDIVECOMPUTERDIR)/lib/libdivecomputer.a
else ifneq ($(strip $(libdc-usr64)),)
	LIBDIVECOMPUTERDIR = /usr
	LIBDIVECOMPUTERINCLUDES = -I$(LIBDIVECOMPUTERDIR)/include/libdivecomputer
	LIBDIVECOMPUTERARCHIVE = $(LIBDIVECOMPUTERDIR)/lib64/libdivecomputer.a
else
$(error Cannot find libdivecomputer - please edit Makefile)
endif
endif

# Libusb-1.0 is only required if libdivecomputer was built with it.
# And libdivecomputer is only built with it if libusb-1.0 is
# installed. So get libusb if it exists, but don't complain
# about it if it doesn't.
LIBUSB = $(shell $(PKGCONFIG) --libs libusb-1.0 2> /dev/null)

LIBGTK = $(shell $(PKGCONFIG) --libs gtk+-2.0 glib-2.0 gconf-2.0)
LIBDIVECOMPUTERCFLAGS = $(LIBDIVECOMPUTERINCLUDES)
LIBDIVECOMPUTER = $(LIBDIVECOMPUTERARCHIVE) $(LIBUSB)

LIBS = $(LIBXML2) $(LIBGTK) $(LIBDIVECOMPUTER) -lpthread

OBJS =	main.o dive.o profile.o info.o equipment.o divelist.o \
	parse-xml.o save-xml.o libdivecomputer.o print.o uemis.o \
	gtk-gui.o statistics.o $(RESFILE)

$(NAME): $(OBJS)
	$(CC) $(LDFLAGS) -o $(NAME) $(OBJS) $(LIBS)

install: $(NAME)
	$(INSTALL) -d -m 755 $(BINDIR)
	$(INSTALL) $(NAME) $(BINDIR)
	$(INSTALL) -d -m 755 $(DESKTOPDIR)
	$(INSTALL) $(DESKTOPFILE) $(DESKTOPDIR)
	$(INSTALL) -d -m 755 $(ICONDIR)
	$(INSTALL) $(ICONFILE) $(ICONDIR)
	@-if test -z "$(DESTDIR)"; then \
		$(gtk_update_icon_cache); \
	fi
	$(INSTALL) -d -m 755 $(MANDIR)
	$(INSTALL) -m 644 $(MANFILES) $(MANDIR)

LIBXML2 = $(shell $(XML2CONFIG) --libs)
XML2CFLAGS = $(shell $(XML2CONFIG) --cflags)
GLIB2CFLAGS = $(shell $(PKGCONFIG) --cflags glib-2.0)
GCONF2CFLAGS =  $(shell $(PKGCONFIG) --cflags gconf-2.0)
GTK2CFLAGS = $(shell $(PKGCONFIG) --cflags gtk+-2.0)

install-macosx: $(NAME)
	$(INSTALL) -d -m 755 $(MACOSXINSTALL)/Contents/Resources
	$(INSTALL) -d -m 755 $(MACOSXINSTALL)/Contents/MacOS
	$(INSTALL) $(NAME) $(MACOSXINSTALL)/Contents/MacOS/
	$(INSTALL) $(MACOSXFILES)/subsurface.sh $(MACOSXINSTALL)/Contents/MacOS/
	$(INSTALL) $(MACOSXFILES)/PkgInfo $(MACOSXINSTALL)/Contents/
	$(INSTALL) $(MACOSXFILES)/Info.plist $(MACOSXINSTALL)/Contents/
	$(INSTALL) $(ICONFILE) $(MACOSXINSTALL)/Contents/Resources/
	$(INSTALL) $(MACOSXFILES)/Subsurface.icns $(MACOSXINSTALL)/Contents/Resources/

parse-xml.o: parse-xml.c dive.h
	$(CC) $(CFLAGS) $(GLIB2CFLAGS) -c $(XML2CFLAGS)  parse-xml.c

save-xml.o: save-xml.c dive.h
	$(CC) $(CFLAGS) $(GLIB2CFLAGS) -c save-xml.c

dive.o: dive.c dive.h
	$(CC) $(CFLAGS) $(GLIB2CFLAGS) -c dive.c

main.o: main.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) $(GLIB2CFLAGS) $(GCONF2CFLAGS) -c main.c

profile.o: profile.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) $(GTK2CFLAGS) $(GLIB2CFLAGS) -c profile.c

info.o: info.c dive.h display.h display-gtk.h divelist.h
	$(CC) $(CFLAGS) $(GTK2CFLAGS) $(GLIB2CFLAGS) -c info.c

equipment.o: equipment.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) $(GTK2CFLAGS) $(GLIB2CFLAGS) -c equipment.c

statistics.o: statistics.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) $(GTK2CFLAGS) $(GLIB2CFLAGS) -c statistics.c

divelist.o: divelist.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) $(GTK2CFLAGS) $(GLIB2CFLAGS) -c divelist.c

print.o: print.c dive.h display.h display-gtk.h
	$(CC) $(CFLAGS) $(GTK2CFLAGS) $(GLIB2CFLAGS) -c print.c

libdivecomputer.o: libdivecomputer.c dive.h display.h display-gtk.h libdivecomputer.h
	$(CC) $(CFLAGS) $(GTK2CFLAGS) $(GLIB2CFLAGS) \
			$(LIBDIVECOMPUTERCFLAGS) \
			-c libdivecomputer.c

gtk-gui.o: gtk-gui.c dive.h display.h divelist.h display-gtk.h libdivecomputer.h Makefile
	$(CC) $(CFLAGS) $(GTK2CFLAGS) $(GLIB2CFLAGS) $(GCONF2CFLAGS) \
			$(LIBDIVECOMPUTERCFLAGS) \
			-DVERSION_STRING='"v$(VERSION)"' \
			-c gtk-gui.c

uemis.o: uemis.c uemis.h
	$(CC) $(CFLAGS) $(GTK2CFLAGS) $(GLIB2CFLAGS) -c uemis.c

clean:
	rm -f $(OBJS) *~ $(NAME)
