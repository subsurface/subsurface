include Configure.mk
VERSION=3.0.2

CC=gcc
CFLAGS=-Wall -Wno-pointer-sign -g $(CLCFLAGS) -DGSEAL_ENABLE
CXX=g++
CXXFLAGS=-Wall -g $(CLCFLAGS) -DQT_NO_KEYWORDS
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
XSLTDIR = $(DATADIR)/subsurface/xslt
gtk_update_icon_cache = gtk-update-icon-cache -f -t $(ICONPATH)

NAME = subsurface
ICONFILE = $(NAME)-icon.svg
DESKTOPFILE = $(NAME).desktop
MANFILES = $(NAME).1
XSLTFILES = xslt/*.xslt

EXTRA_FLAGS =  $(QTCXXFLAGS) $(GTKCFLAGS) $(GLIB2CFLAGS) $(XML2CFLAGS) \
	       $(LIBDIVECOMPUTERCFLAGS) \
	       $(LIBSOUPCFLAGS) $(GCONF2CFLAGS)

HEADERS = \
	qt-ui/addcylinderdialog.h \
	qt-ui/divelistview.h \
	qt-ui/maintab.h \
	qt-ui/mainwindow.h \
	qt-ui/models.h \
	qt-ui/plotareascene.h \
	qt-ui/starwidget.h \


QTOBJS = qt-ui/maintab.o  qt-ui/mainwindow.o  qt-ui/plotareascene.o qt-ui/divelistview.o \
	qt-ui/addcylinderdialog.o qt-ui/models.o qt-ui/starwidget.o

GTKOBJS = info-gtk.o divelist-gtk.o planner-gtk.o statistics-gtk.o

OBJS =	main.o dive.o time.o profile.o info.o equipment.o divelist.o divelist-gtk.o deco.o \
	planner.o planner-gtk.o \
	parse-xml.o save-xml.o libdivecomputer.o print.o uemis.o uemis-downloader.o \
	qt-gui.o statistics.o file.o cochran.o device.o download-dialog.o prefs.o \
	webservice.o sha1.o $(RESFILE) $(QTOBJS) $(GTKOBJS)

ifneq ($(SQLITE3FLAGS),)
	EXTRA_FLAGS += -DSQLITE3 $(SQLITE3FLAGS)
endif
ifneq ($(ZIPFLAGS),)
       EXTRA_FLAGS += -DLIBZIP $(ZIPFLAGS)
endif
ifneq ($(strip $(LIBXSLT)),)
       EXTRA_FLAGS += -DXSLT='"$(XSLTDIR)"' $(XSLCFLAGS)
endif
ifneq ($(strip $(LIBOSMGPSMAP)),)
       OBJS += gps.o
       EXTRA_FLAGS += -DHAVE_OSM_GPS_MAP $(OSMGPSMAPFLAGS)
endif

ifneq (,$(filter $(UNAME),linux kfreebsd gnu))
	OBJS += linux.o
else ifeq ($(UNAME), darwin)
	OBJS += macos.o
	MACOSXINSTALL = /Applications/Subsurface.app
	MACOSXFILES = packaging/macosx
	MACOSXSTAGING = $(MACOSXFILES)/Subsurface.app
	INFOPLIST = $(MACOSXFILES)/Info.plist
	INFOPLISTINPUT = $(INFOPLIST).in
	LDFLAGS += -headerpad_max_install_names -sectcreate __TEXT __info_plist $(INFOPLIST)
else
	OBSJ += windows.o
	WINDOWSSTAGING = ./packaging/windows
	WINMSGDIRS=$(addprefix share/locale/,$(shell ls po/*.po | sed -e 's/po\/\(..\)_.*/\1\/LC_MESSAGES/'))
	NSIINPUTFILE = $(WINDOWSSTAGING)/subsurface.nsi.in
	NSIFILE = $(WINDOWSSTAGING)/subsurface.nsi
	MAKENSIS = makensis
	XSLTDIR = .\\xslt
endif

LIBS = $(LIBQT) $(LIBXML2) $(LIBXSLT) $(LIBSQLITE3) $(LIBGTK) $(LIBGCONF2) $(LIBDIVECOMPUTER) \
	$(EXTRALIBS) $(LIBZIP) -lpthread -lm $(LIBOSMGPSMAP) $(LIBSOUP) $(LIBWINSOCK)

MSGLANGS=$(notdir $(wildcard po/*.po))

# Add files to the following variables if the auto-detection based on the
# filename fails
OBJS_NEEDING_MOC =
OBJS_NEEDING_UIC =
HEADERS_NEEDING_MOC =

include Rules.mk
