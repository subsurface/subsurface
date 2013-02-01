VERSION=2.9

CC=gcc
CFLAGS=-Wall -Wno-pointer-sign -g $(CLCFLAGS) -DGSEAL_ENABLE
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
VERSION_STRING := $(shell git describe --tags --abbrev=12 || echo "v$(VERSION)")
PRODVERSION_STRING := $(shell git describe --tags --abbrev=12 | sed 's/v\([0-9]*\)\.\([0-9]*\)-\([0-9]*\)-.*/\1.\2.\3.0/ ; s/v\([0-9]\)\.\([0-9]*\)/\1.\2.0.0/' || echo "$(VERSION).0.0")

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

ifneq ($(LIBDCDEVEL),)
	LIBDIVECOMPUTERDIR = ../libdivecomputer
	LIBDIVECOMPUTERINCLUDES = -I$(LIBDIVECOMPUTERDIR)/include
	LIBDIVECOMPUTERARCHIVE = $(LIBDIVECOMPUTERDIR)/src/.libs/libdivecomputer.a
else ifneq ($(strip $(libdc-local)),)
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
GTKCFLAGS = $(shell $(PKGCONFIG) --cflags gtk+-2.0)
CFLAGS += $(shell $(XSLCONFIG) --cflags)
OSMGPSMAPFLAGS += $(shell $(PKGCONFIG) --cflags osmgpsmap 2> /dev/null)
LIBOSMGPSMAP += $(shell $(PKGCONFIG) --libs osmgpsmap 2> /dev/null)
ifneq ($(strip $(LIBOSMGPSMAP)),)
	GPSOBJ = gps.o
	CFLAGS += -DHAVE_OSM_GPS_MAP
endif
LIBSOUPCFLAGS = $(shell $(PKGCONFIG) --cflags libsoup-2.4)
LIBSOUP = $(shell $(PKGCONFIG) --libs libsoup-2.4)

LIBZIP = $(shell $(PKGCONFIG) --libs libzip 2> /dev/null)
ifneq ($(strip $(LIBZIP)),)
	ZIP = -DLIBZIP $(shell $(PKGCONFIG) --cflags libzip)
endif

ifeq ($(UNAME), linux)
	LIBGCONF2 = $(shell $(PKGCONFIG) --libs gconf-2.0)
	GCONF2CFLAGS =  $(shell $(PKGCONFIG) --cflags gconf-2.0)
	OSSUPPORT = linux
	OSSUPPORT_CFLAGS = $(GTKCFLAGS) $(GCONF2CFLAGS)
else ifeq ($(UNAME), darwin)
	OSSUPPORT = macos
	OSSUPPORT_CFLAGS = $(GTKCFLAGS)
	MACOSXINSTALL = /Applications/Subsurface.app
	MACOSXFILES = packaging/macosx
	MACOSXSTAGING = $(MACOSXFILES)/Subsurface.app
	EXTRALIBS = $(shell $(PKGCONFIG) --libs gtk-mac-integration) -framework CoreFoundation
	CFLAGS += $(shell $(PKGCONFIG) --cflags gtk-mac-integration)
	LDFLAGS += -headerpad_max_install_names
	GTK_MAC_BUNDLER = ~/.local/bin/gtk-mac-bundler
else
	OSSUPPORT = windows
	OSSUPPORT_CFLAGS = $(GTKCFLAGS)
	WINDOWSSTAGING = ./packaging/windows
	WINMSGDIRS=$(addprefix share/locale/,$(shell ls po/*.po | sed -e 's/po\/\(..\)_.*/\1\/LC_MESSAGES/'))
	NSIINPUTFILE = $(WINDOWSSTAGING)/subsurface.nsi.in
	NSIFILE = $(WINDOWSSTAGING)/subsurface.nsi
	MAKENSIS = makensis

endif

ifneq ($(strip $(LIBXSLT)),)
	# We still need proper paths and install options for OSX and Windows
	ifeq ($(shell sh -c 'uname -s 2>/dev/null || echo not'),Linux)
		XSLT=-DXSLT='"$(XSLTDIR)"'
	endif
endif

LIBS = $(LIBXML2) $(LIBXSLT) $(LIBGTK) $(LIBGCONF2) $(LIBDIVECOMPUTER) $(EXTRALIBS) $(LIBZIP) -lpthread -lm -lssl -lcrypto $(LIBOSMGPSMAP) $(LIBSOUP)

MSGLANGS=$(notdir $(wildcard po/*po))
MSGOBJS=$(addprefix share/locale/,$(MSGLANGS:.po=.UTF-8/LC_MESSAGES/subsurface.mo))

OBJS =	main.o dive.o time.o profile.o info.o equipment.o divelist.o deco.o planner.o \
	parse-xml.o save-xml.o libdivecomputer.o print.o uemis.o uemis-downloader.o \
	gtk-gui.o statistics.o file.o cochran.o device.o download-dialog.o prefs.o \
	webservice.o $(GPSOBJ) $(OSSUPPORT).o $(RESFILE)

DEPS = $(wildcard .dep/*.dep)

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

create-windows-installer: $(NAME) $(NSIFILE) install-cross-windows
	$(MAKENSIS) $(NSIFILE)

$(NSIFILE): $(NSIINPUTFILE) Makefile
	$(shell cat $(NSIINPUTFILE) | sed -e 's/VERSIONTOKEN/$(VERSION_STRING)/;s/PRODVTOKEN/$(PRODVERSION_STRING)/' > $(NSIFILE))


update-po-files:
	xgettext -o po/subsurface-new.pot -s -k_ -kN_ --keyword=C_:1c,2  --add-comments="++GETTEXT" *.c
	for i in po/*.po; do \
		msgmerge -s -U $$i po/subsurface-new.pot ; \
	done

EXTRA_FLAGS =	$(GTKCFLAGS) $(GLIB2CFLAGS) $(XML2CFLAGS) \
		$(XSLT) $(ZIP) $(LIBDIVECOMPUTERCFLAGS) \
		$(LIBSOUPCFLAGS) $(OSMGPSMAPFLAGS) $(GCONF2CFLAGS) \
		-DVERSION_STRING='"$(VERSION_STRING)"'

%.o: %.c
	@echo '    CC' $<
	@mkdir -p .dep
	@$(CC) $(CFLAGS) $(EXTRA_FLAGS) -MD -MF .dep/$@.dep -c -o $@ $<

share/locale/%.UTF-8/LC_MESSAGES/subsurface.mo: po/%.po po/%.aliases
	mkdir -p $(dir $@)
	msgfmt -c -o $@ po/$*.po
	@-if test -s po/$*.aliases; then \
		for ALIAS in `cat po/$*.aliases`; do \
			mkdir -p share/locale/$$ALIAS/LC_MESSAGES; \
			cp $@ share/locale/$$ALIAS/LC_MESSAGES; \
		done; \
	fi

# this should work but it doesn't preserve the transparancy - so I manually converted with gimp
# satellite.png: satellite.svg
#      convert -resize 11x16 -depth 8 satellite.svg satellite.png
#
# the following creates the pixbuf data in .h files with the basename followed by '_pixmap'
# as name of the data structure
%.h: %.png
	@echo '    gdk-pixbuf-csource' $<
	@gdk-pixbuf-csource --struct --name $*_pixbuf $< > $@

doc:
	$(MAKE) -C Documentation doc

clean:
	rm -f $(OBJS) *~ $(NAME) $(NAME).exe po/*~ po/subsurface-new.pot
	rm -rf share .dep

-include $(DEPS)
