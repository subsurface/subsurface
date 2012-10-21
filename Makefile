VERSION=2.1-rc3

CC=gcc
CFLAGS=-Wall -Wno-pointer-sign -g
INSTALL=install
PKGCONFIG=pkg-config
XML2CONFIG=xml2-config
XSLCONFIG=xslt-config

# these locations seem to work for SuSE and Fedora
# prefix = $(HOME)
prefix = $(DESTDIR)/usr
BINDIR = $(prefix)/bin
DATADIR = $(prefix)/share
DESKTOPDIR = $(DATADIR)/applications
ICONPATH = $(DATADIR)/icons/hicolor
ICONDIR = $(ICONPATH)/scalable/apps
MANDIR = $(DATADIR)/man/man1
XSLTDIR = $(DATADIR)/subsurface/xslt
gtk_update_icon_cache = gtk-update-icon-cache -f -t $(ICONPATH)

NAME = subsurface
ICONFILE = $(NAME).svg
DESKTOPFILE = $(NAME).desktop
MANFILES = $(NAME).1
XSLTFILES = xslt/*.xslt

UNAME := $(shell $(CC) -dumpmachine 2>&1 | grep -E -o "linux|darwin|win")

# find libdivecomputer
# First deal with the cross compile environment and with Mac.
# For the native case, Linus doesn't want to trust pkg-config given
# how young libdivecomputer still is - so we check the typical
# subdirectories of /usr/local and /usr and then we give up. You can
# override by simply setting it here
#
ifeq ($(CC), i686-w64-mingw32-gcc)
# ok, we are cross building for Windows
	LIBDIVECOMPUTERINCLUDES = $(shell $(PKGCONFIG) --cflags libdivecomputer)
	LIBDIVECOMPUTERARCHIVE = $(shell $(PKGCONFIG) --libs libdivecomputer)
	RESFILE = packaging/windows/subsurface.res
	LDFLAGS += -Wl,-subsystem,windows
else ifeq ($(UNAME), darwin)
	LIBDIVECOMPUTERINCLUDES = $(shell $(PKGCONFIG) --cflags libdivecomputer)
	LIBDIVECOMPUTERARCHIVE = $(shell $(PKGCONFIG) --libs libdivecomputer)
else
libdc-local := $(wildcard /usr/local/lib/libdivecomputer.a)
libdc-local64 := $(wildcard /usr/local/lib64/libdivecomputer.a)
libdc-usr := $(wildcard /usr/lib/libdivecomputer.a)
libdc-usr64 := $(wildcard /usr/lib64/libdivecomputer.a)

ifneq ($(strip $(libdc-local)),)
	LIBDIVECOMPUTERDIR = /usr/local
	LIBDIVECOMPUTERINCLUDES = -I$(LIBDIVECOMPUTERDIR)/include
	LIBDIVECOMPUTERARCHIVE = $(LIBDIVECOMPUTERDIR)/lib/libdivecomputer.a
else ifneq ($(strip $(libdc-local64)),)
	LIBDIVECOMPUTERDIR = /usr/local
	LIBDIVECOMPUTERINCLUDES = -I$(LIBDIVECOMPUTERDIR)/include
	LIBDIVECOMPUTERARCHIVE = $(LIBDIVECOMPUTERDIR)/lib64/libdivecomputer.a
else ifneq ($(strip $(libdc-usr)),)
	LIBDIVECOMPUTERDIR = /usr
	LIBDIVECOMPUTERINCLUDES = -I$(LIBDIVECOMPUTERDIR)/include
	LIBDIVECOMPUTERARCHIVE = $(LIBDIVECOMPUTERDIR)/lib/libdivecomputer.a
else ifneq ($(strip $(libdc-usr64)),)
	LIBDIVECOMPUTERDIR = /usr
	LIBDIVECOMPUTERINCLUDES = -I$(LIBDIVECOMPUTERDIR)/include
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

LIBGTK = $(shell $(PKGCONFIG) --libs gtk+-2.0 glib-2.0)
LIBDIVECOMPUTERCFLAGS = $(LIBDIVECOMPUTERINCLUDES)
LIBDIVECOMPUTER = $(LIBDIVECOMPUTERARCHIVE) $(LIBUSB)

LIBXML2 = $(shell $(XML2CONFIG) --libs)
LIBXSLT = $(shell $(XSLCONFIG) --libs)
XML2CFLAGS = $(shell $(XML2CONFIG) --cflags)
GLIB2CFLAGS = $(shell $(PKGCONFIG) --cflags glib-2.0)
GTK2CFLAGS = $(shell $(PKGCONFIG) --cflags gtk+-2.0)
CFLAGS += $(shell $(XSLCONFIG) --cflags)

LIBZIP = $(shell $(PKGCONFIG) --libs libzip 2> /dev/null)
ifneq ($(strip $(LIBZIP)),)
	ZIP = -DLIBZIP $(shell $(PKGCONFIG) --cflags libzip)
endif

ifeq ($(UNAME), linux)
	LIBGCONF2 = $(shell $(PKGCONFIG) --libs gconf-2.0)
	GCONF2CFLAGS =  $(shell $(PKGCONFIG) --cflags gconf-2.0)
	OSSUPPORT = linux
	OSSUPPORT_CFLAGS = $(GTK2CFLAGS) $(GCONF2CFLAGS)
else ifeq ($(UNAME), darwin)
	OSSUPPORT = macos
	OSSUPPORT_CFLAGS = $(GTK2CFLAGS)
	MACOSXINSTALL = /Applications/Subsurface.app
	MACOSXFILES = packaging/macosx
	MACOSXSTAGING = $(MACOSXFILES)/Subsurface.app
	EXTRALIBS = $(shell $(PKGCONFIG) --libs gtk-mac-integration) -framework CoreFoundation
	CFLAGS += $(shell $(PKGCONFIG) --cflags gtk-mac-integration)
	LDFLAGS += -headerpad_max_install_names
	GTK_MAC_BUNDLER = ~/.local/bin/gtk-mac-bundler
else
	OSSUPPORT = windows
	OSSUPPORT_CFLAGS = $(GTK2CFLAGS)
	WINDOWSSTAGING = ./packaging/windows
	WINMSGDIRS=$(addprefix share/locale/,$(shell ls po/*.po | sed -e 's/po\/\(..\)_.*/\1\/LC_MESSAGES/'))
endif

ifneq ($(strip $(LIBXSLT)),)
	# We still need proper paths and install options for OSX and Windows
	ifeq ($(shell sh -c 'uname -s 2>/dev/null || echo not'),Linux)
		XSLT=-DXSLT='"$(XSLTDIR)"'
	endif
endif

LIBS = $(LIBXML2) $(LIBXSLT) $(LIBGTK) $(LIBGCONF2) $(LIBDIVECOMPUTER) $(EXTRALIBS) $(LIBZIP) -lpthread -lm

MSGLANGS=$(notdir $(wildcard po/*po))
MSGOBJS=$(addprefix share/locale/,$(MSGLANGS:.po=.UTF-8/LC_MESSAGES/subsurface.mo))

OBJS =	main.o dive.o time.o profile.o info.o equipment.o divelist.o \
	parse-xml.o save-xml.o libdivecomputer.o print.o uemis.o uemis-downloader.o \
	gtk-gui.o statistics.o file.o cochran.o $(OSSUPPORT).o $(RESFILE)

$(NAME): $(OBJS) $(MSGOBJS)
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
	@-if test ! -z "$(XSLT)"; then \
		$(INSTALL) -d -m 755 $(DATADIR)/subsurface; \
		$(INSTALL) -d -m 755 $(XSLTDIR); \
		$(INSTALL) -m 644 $(XSLTFILES) $(XSLTDIR); \
	fi
	for LOC in $(wildcard share/locale/*/LC_MESSAGES); do \
		$(INSTALL) -d -m 755 $(prefix)/$$LOC; \
		$(INSTALL) $$LOC/subsurface.mo $(prefix)/$$LOC/subsurface.mo; \
	done


install-macosx: $(NAME)
	$(INSTALL) -d -m 755 $(MACOSXINSTALL)/Contents/Resources
	$(INSTALL) -d -m 755 $(MACOSXINSTALL)/Contents/MacOS
	$(INSTALL) $(NAME) $(MACOSXINSTALL)/Contents/MacOS/$(NAME)-bin
	$(INSTALL) $(MACOSXFILES)/$(NAME).sh $(MACOSXINSTALL)/Contents/MacOS/$(NAME)
	$(INSTALL) $(MACOSXFILES)/PkgInfo $(MACOSXINSTALL)/Contents/
	$(INSTALL) $(MACOSXFILES)/Info.plist $(MACOSXINSTALL)/Contents/
	$(INSTALL) $(ICONFILE) $(MACOSXINSTALL)/Contents/Resources/
	$(INSTALL) $(MACOSXFILES)/Subsurface.icns $(MACOSXINSTALL)/Contents/Resources/
	for LOC in $(wildcard share/locale/*/LC_MESSAGES); do \
		$(INSTALL) -d -m 755 $(MACOSXINSTALL)/Contents/Resources/$$LOC; \
		$(INSTALL) $$LOC/subsurface.mo $(MACOSXINSTALL)/Contents/Resources/$$LOC/subsurface.mo; \
	done

create-macosx-bundle: $(NAME)
	$(INSTALL) -d -m 755 $(MACOSXSTAGING)/Contents/Resources
	$(INSTALL) -d -m 755 $(MACOSXSTAGING)/Contents/MacOS
	$(INSTALL) $(NAME) $(MACOSXSTAGING)/Contents/MacOS/
	$(INSTALL) $(MACOSXFILES)/PkgInfo $(MACOSXSTAGING)/Contents/
	$(INSTALL) $(MACOSXFILES)/Info.plist $(MACOSXSTAGING)/Contents/
	$(INSTALL) $(ICONFILE) $(MACOSXSTAGING)/Contents/Resources/
	$(INSTALL) $(MACOSXFILES)/Subsurface.icns $(MACOSXSTAGING)/Contents/Resources/
	for LOC in $(wildcard share/locale/*/LC_MESSAGES); do \
		$(INSTALL) -d -m 755 $(MACOSXSTAGING)/Contents/Resources/$$LOC; \
		$(INSTALL) $$LOC/subsurface.mo $(MACOSXSTAGING)/Contents/Resources/$$LOC/subsurface.mo; \
	done
	$(GTK_MAC_BUNDLER) packaging/macosx/subsurface.bundle

install-cross-windows: $(NAME)
	$(INSTALL) -d -m 755 $(WINDOWSSTAGING)/share/locale
	for MSG in $(WINMSGDIRS); do\
		$(INSTALL) -d -m 755 $(WINDOWSSTAGING)/$$MSG;\
		$(INSTALL) $(CROSS_PATH)/$$MSG/* $(WINDOWSSTAGING)/$$MSG;\
	done
	for LOC in $(wildcard share/locale/*/LC_MESSAGES); do \
		$(INSTALL) -d -m 755 $(WINDOWSSTAGING)/$$LOC; \
		$(INSTALL) $$LOC/subsurface.mo $(WINDOWSSTAGING)/$$LOC/subsurface.mo; \
	done

update-po-files:
	xgettext -o po/subsurface-new.pot -s -k_ -kN_ --keyword=C_:1c,2  --add-comments="++GETTEXT" *.c
	for i in po/*.po; do \
		msgmerge -s -U $$i po/subsurface-new.pot ; \
	done

file.o: file.c dive.h file.h
	$(CC) $(CFLAGS) $(GLIB2CFLAGS) $(XML2CFLAGS) $(XSLT) $(ZIP) -c file.c

cochran.o: cochran.c dive.h file.h
	$(CC) $(CFLAGS) $(GLIB2CFLAGS) $(XML2CFLAGS) $(XSLT) $(ZIP) -c cochran.c

parse-xml.o: parse-xml.c dive.h
	$(CC) $(CFLAGS) $(GLIB2CFLAGS) $(XML2CFLAGS) $(XSLT) -c parse-xml.c

save-xml.o: save-xml.c dive.h
	$(CC) $(CFLAGS) $(GLIB2CFLAGS) $(XML2CFLAGS) -c save-xml.c

dive.o: dive.c dive.h
	$(CC) $(CFLAGS) $(GLIB2CFLAGS) $(XML2CFLAGS) -c dive.c

time.o: time.c dive.h
	$(CC) $(CFLAGS) $(GLIB2CFLAGS) $(XML2CFLAGS) -c time.c

main.o: main.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) $(GLIB2CFLAGS) $(GCONF2CFLAGS) $(XML2CFLAGS) -c main.c

profile.o: profile.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) $(GTK2CFLAGS) $(GLIB2CFLAGS) $(XML2CFLAGS) -c profile.c

info.o: info.c dive.h display.h display-gtk.h divelist.h
	$(CC) $(CFLAGS) $(GTK2CFLAGS) $(GLIB2CFLAGS) $(XML2CFLAGS) -c info.c

equipment.o: equipment.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) $(GTK2CFLAGS) $(GLIB2CFLAGS) $(XML2CFLAGS) -c equipment.c

statistics.o: statistics.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) $(GTK2CFLAGS) $(GLIB2CFLAGS) $(XML2CFLAGS) -c statistics.c

divelist.o: divelist.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) $(GTK2CFLAGS) $(GLIB2CFLAGS) $(XML2CFLAGS) -c divelist.c

print.o: print.c dive.h display.h display-gtk.h
	$(CC) $(CFLAGS) $(GTK2CFLAGS) $(GLIB2CFLAGS) $(XML2CFLAGS) -c print.c

libdivecomputer.o: libdivecomputer.c dive.h display.h display-gtk.h libdivecomputer.h
	$(CC) $(CFLAGS) $(GTK2CFLAGS) $(GLIB2CFLAGS) $(XML2CFLAGS) \
			$(LIBDIVECOMPUTERCFLAGS) \
			-c libdivecomputer.c

gtk-gui.o: gtk-gui.c dive.h display.h divelist.h display-gtk.h libdivecomputer.h Makefile
	$(CC) $(CFLAGS) $(GTK2CFLAGS) $(GLIB2CFLAGS) $(GCONF2CFLAGS) $(XML2CFLAGS) \
			$(LIBDIVECOMPUTERCFLAGS) \
			-DVERSION_STRING='"v$(VERSION)"' \
			-c gtk-gui.c

uemis.o: uemis.c dive.h uemis.h
	$(CC) $(CFLAGS) $(GTK2CFLAGS) $(GLIB2CFLAGS) $(XML2CFLAGS) -c uemis.c

uemis-downloader.o: uemis-downloader.c dive.h uemis.h
	$(CC) $(CFLAGS) $(GTK2CFLAGS) $(GLIB2CFLAGS) $(XML2CFLAGS) -c uemis-downloader.c

$(OSSUPPORT).o: $(OSSUPPORT).c display-gtk.h
	$(CC) $(CFLAGS) $(OSSUPPORT_CFLAGS) -c $(OSSUPPORT).c

share/locale/%.UTF-8/LC_MESSAGES/subsurface.mo: po/%.po po/%.aliases
	mkdir -p $(dir $@)
	msgfmt -c -o $@ po/$*.po
	@-if test -s po/$*.aliases; then \
		for ALIAS in `cat po/$*.aliases`; do \
			mkdir -p share/locale/$$ALIAS/LC_MESSAGES; \
			cp $@ share/locale/$$ALIAS/LC_MESSAGES; \
		done; \
	fi

doc:
	$(MAKE) -C Documentation doc

clean:
	rm -f $(OBJS) *~ $(NAME) $(NAME).exe po/*~ po/subsurface-new.pot
	rm -rf share
