# -*- Makefile -*-
# This file contains the detection rules
all:

PKGCONFIG=pkg-config
XML2CONFIG=xml2-config
XSLCONFIG=xslt-config
QMAKE=qmake-qt4
MOC=moc
UIC=uic

CONFIGFILE = config.cache
ifeq ($(CONFIGURING),1)

# Detect the target system
# Ask the compiler what OS it's producing files for
UNAME := $(shell $(CC) -dumpmachine 2>&1 | grep -E -o "linux|darwin|win|gnu|kfreebsd")

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
	LIBWINSOCK = -lwsock32
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

# Use qmake to find out which Qt version we are building for.
QT_VERSION_MAJOR = $(shell $(QMAKE) -query QT_VERSION | cut -d. -f1)
ifeq ($(QT_VERSION_MAJOR), 5)
	QT_MODULES = Qt5Widgets Qt5Svg
	QT_CORE = Qt5Core
	QTBINDIR = $(shell $(QMAKE) -query QT_HOST_BINS)
else
	QT_MODULES = QtGui QtSvg
	QT_CORE = QtCore
	QTBINDIR = $(shell $(QMAKE) -query QT_INSTALL_BINS)
endif

# we need GLIB2CFLAGS for gettext
QTCXXFLAGS = $(shell $(PKGCONFIG) --cflags $(QT_MODULES)) $(GLIB2CFLAGS)
LIBQT = $(shell $(PKGCONFIG) --libs $(QT_MODULES))
ifneq ($(filter reduce_relocations, $(shell $(PKGCONFIG) --variable qt_config $(QT_CORE))), )
	QTCXXFLAGS += -fPIE
endif
MOC = $(QTBINDIR)/moc
UIC = $(QTBINDIR)/uic
RCC = $(QTBINDIR)/rcc

LIBGTK = $(shell $(PKGCONFIG) --libs gtk+-2.0 glib-2.0)
ifneq (,$(filter $(UNAME),linux kfreebsd gnu))
	LIBGCONF2 = $(shell $(PKGCONFIG) --libs gconf-2.0)
	GCONF2CFLAGS =  $(shell $(PKGCONFIG) --cflags gconf-2.0)
else ifeq ($(UNAME), darwin)
	LIBGTK += $(shell $(PKGCONFIG) --libs gtk-mac-integration) -framework CoreFoundation -framework CoreServices
	GTKCFLAGS += $(shell $(PKGCONFIG) --cflags gtk-mac-integration)
	GTK_MAC_BUNDLER = ~/.local/bin/gtk-mac-bundler
endif

LIBDIVECOMPUTERCFLAGS = $(LIBDIVECOMPUTERINCLUDES)
LIBDIVECOMPUTER = $(LIBDIVECOMPUTERARCHIVE) $(LIBUSB)

LIBXML2 = $(shell $(XML2CONFIG) --libs)
LIBXSLT = $(shell $(XSLCONFIG) --libs)
XML2CFLAGS = $(shell $(XML2CONFIG) --cflags)
GLIB2CFLAGS = $(shell $(PKGCONFIG) --cflags glib-2.0)
GTKCFLAGS += $(shell $(PKGCONFIG) --cflags gtk+-2.0)
XSLCFLAGS = $(shell $(XSLCONFIG) --cflags)
OSMGPSMAPFLAGS += $(shell $(PKGCONFIG) --cflags osmgpsmap 2> /dev/null)
LIBOSMGPSMAP += $(shell $(PKGCONFIG) --libs osmgpsmap 2> /dev/null)
LIBSOUPCFLAGS = $(shell $(PKGCONFIG) --cflags libsoup-2.4)
LIBSOUP = $(shell $(PKGCONFIG) --libs libsoup-2.4)

LIBZIP = $(shell $(PKGCONFIG) --libs libzip 2> /dev/null)
ZIPFLAGS = $(strip $(shell $(PKGCONFIG) --cflags libzip 2> /dev/null))

LIBSQLITE3 = $(shell $(PKGCONFIG) --libs sqlite3 2> /dev/null)
SQLITE3FLAGS = $(strip $(shell $(PKGCONFIG) --cflags sqlite3))

# Write the configure file
all: configure
configure $(CONFIGURE): Configure.mk
	@echo "\
	CONFIGURED = 1\\\
	UNAME = $(UNAME)\\\
	LIBDIVECOMPUTERDIR = $(LIBDIVECOMPUTERDIR)\\\
	LIBDIVECOMPUTERCFLAGS = $(LIBDIVECOMPUTERCFLAGS)\\\
	LIBDIVECOMPUTER = $(LIBDIVECOMPUTER)\\\
	LIBWINSOCK = $(LIBWINSOCK)\\\
	LDFLAGS = $(LDFLAGS)\\\
	RESFILE = $(RESFILE)\\\
	LIBQT = $(LIBQT)\\\
	QTCXXFLAGS = $(QTCXXFLAGS)\\\
	MOC = $(MOC)\\\
	UIC = $(UIC)\\\
	RCC = $(RCC)\\\
	LIBGTK = $(LIBGTK)\\\
	GTKCFLAGS = $(GTKCFLAGS)\\\
	LIBGCONF2 = $(LIBGCONF2)\\\
	GCONF2CFLAGS = $(GCONF2CFLAGS)\\\
	GTK_MAC_BUNDLER = $(GTK_MAC_BUNDLER)\\\
	LIBXML2 = $(LIBXML2)\\\
	LIBXSLT = $(LIBXSLT)\\\
	XML2CFLAGS = $(XML2CFLAGS)\\\
	GLIB2CFLAGS = $(GLIB2CFLAGS)\\\
	XSLCFLAGS = $(XSLCFLAGS)\\\
	OSMGPSMAPFLAGS = $(OSMGPSMAPFLAGS)\\\
	LIBOSMGPSMAP = $(LIBOSMGPSMAP)\\\
	LIBSOUPCFLAGS = $(LIBSOUPCFLAGS)\\\
	LIBSOUP = $(LIBSOUP)\\\
	LIBZIP = $(LIBZIP)\\\
	ZIPFLAGS = $(ZIPFLAGS)\\\
	LIBSQLITE3 = $(LIBSQLITE3)\\\
	SQLITE3FLAGS = $(SQLITE3FLAGS)\\\
	" | tr '\\' '\n' > $(CONFIGFILE)

else
configure $(CONFIGFILE): Configure.mk
	@test -e $(CONFIGFILE) && echo Reconfiguring.. || echo Configuring...
	@$(MAKE) CONFIGURING=1 configure
	@echo Done

-include $(CONFIGFILE)
endif

.PHONY: configure all
