NAME = subsurface
CAPITALIZED_NAME = Subsurface
TARGET = $(NAME)

include Configure.mk
VERSION=3.1

CC=gcc
CFLAGS=-Wall -Wno-pointer-sign -g $(CLCFLAGS) -DGSEAL_ENABLE
CXX=g++
CXXFLAGS=-Wall -g $(CLCXXFLAGS) $(MARBLEFLAGS)
INSTALL=install

# these locations seem to work for SuSE and Fedora
# prefix = $(HOME)
prefix = $(DESTDIR)/usr
BINDIR = $(prefix)/bin
DATADIR = $(prefix)/share
DOCDIR = $(DATADIR)/doc/$(NAME)
DESKTOPDIR = $(DATADIR)/applications
ICONPATH = $(DATADIR)/icons/hicolor
ICONDIR = $(ICONPATH)/scalable/apps
MANDIR = $(DATADIR)/man/man1
XSLTDIR = $(DATADIR)/$(NAME)/xslt
MARBLEDIR = marbledata/maps/earth/googlesat

ICONFILE = $(NAME)-icon.svg
DESKTOPFILE = $(NAME).desktop
MANFILES = $(NAME).1
XSLTFILES = xslt/*.xslt xslt/*.xsl

EXTRA_FLAGS =  $(QTCXXFLAGS) $(GLIB2CFLAGS) $(XML2CFLAGS) \
	       $(LIBDIVECOMPUTERCFLAGS) \
	       $(LIBSOUPCFLAGS) -I. -DQT_NO_STL

HEADERS = \
	qt-ui/divelistview.h \
	qt-ui/maintab.h \
	qt-ui/mainwindow.h \
	qt-ui/models.h \
	qt-ui/plotareascene.h \
	qt-ui/starwidget.h \
	qt-ui/modeldelegates.h \
	qt-ui/profilegraphics.h \
	qt-ui/globe.h \
	qt-ui/kmessagewidget.h \
	qt-ui/downloadfromdivecomputer.h \
	qt-ui/preferences.h \
	qt-ui/simplewidgets.h \
	qt-ui/subsurfacewebservices.h \
	qt-ui/divecomputermanagementdialog.h \
	qt-ui/diveplanner.h \
	qt-ui/about.h \
	qt-ui/graphicsview-common.h \
	qt-ui/printdialog.h \
	qt-ui/printoptions.h \
	qt-ui/printlayout.h \
	qt-ui/completionmodels.h \
	qt-ui/tableview.h


SOURCES = \
	deco.c \
	device.c \
	dive.c \
	divelist.c \
	equipment.c \
	file.c \
	parse-xml.c \
	planner.c \
	subsurfacestartup.c \
	profile.c \
	save-xml.c \
	sha1.c \
	statistics.c \
	time.c \
	uemis.c \
	uemis-downloader.c \
	libdivecomputer.c \
	gettextfromc.cpp \
	qthelper.cpp \
	qt-ui/simplewidgets.cpp \
	qt-ui/tableview.cpp \
	qt-ui/mainwindow.cpp \
	qt-ui/models.cpp \
	qt-ui/plotareascene.cpp \
	qt-ui/starwidget.cpp \
	qt-ui/modeldelegates.cpp \
	qt-ui/profilegraphics.cpp \
	qt-ui/globe.cpp \
	qt-ui/kmessagewidget.cpp \
	qt-ui/downloadfromdivecomputer.cpp \
	qt-ui/preferences.cpp \
	qt-ui/subsurfacewebservices.cpp \
	qt-ui/divecomputermanagementdialog.cpp \
	qt-ui/diveplanner.cpp \
	qt-ui/divelistview.cpp \
	qt-ui/about.cpp \
	qt-ui/graphicsview-common.cpp \
	qt-ui/printdialog.cpp \
	qt-ui/printoptions.cpp \
	qt-ui/printlayout.cpp \
	qt-ui/completionmodels.cpp \
	qt-ui/maintab.cpp \
	qt-gui.cpp \
	main.cpp \
	$(RESFILE)

FORMS = \
	qt-ui/about.ui \
	qt-ui/divecomputermanagementdialog.ui \
	qt-ui/diveplanner.ui \
	qt-ui/downloadfromdivecomputer.ui \
	qt-ui/maintab.ui \
	qt-ui/mainwindow.ui \
	qt-ui/preferences.ui \
	qt-ui/printoptions.ui \
	qt-ui/renumber.ui \
	qt-ui/subsurfacewebservices.ui \
	qt-ui/tableview.ui

RESOURCES = $(NAME).qrc

ifneq ($(SQLITE3FLAGS),)
	EXTRA_FLAGS += -DSQLITE3 $(SQLITE3FLAGS)
endif
ifneq ($(ZIPFLAGS),)
       EXTRA_FLAGS += -DLIBZIP $(ZIPFLAGS)
endif
ifneq ($(strip $(LIBXSLT)),)
       EXTRA_FLAGS += -DXSLT='"$(XSLTDIR)"' $(XSLCFLAGS)
endif

ifneq (,$(filter $(UNAME),linux kfreebsd gnu))
	SOURCES += linux.c
else ifeq ($(UNAME), darwin)
	SOURCES += macos.c
	MACOSXINSTALL = /Applications/$(CAPITALIZED_NAME).app
	MACOSXFILES = packaging/macosx
	MACOSXSTAGING = $(MACOSXFILES)/$(CAPITALIZED_NAME).app
	INFOPLIST = $(MACOSXFILES)/Info.plist
	INFOPLISTINPUT = $(INFOPLIST).in
	LDFLAGS += -headerpad_max_install_names
else
	SOURCES += windows.c
	WINDOWSSTAGING = ./packaging/windows
	WINMSGDIRS=$(addprefix share/locale/,$(shell ls po/*.po | sed -e 's/po\/\(..\)_.*/\1\/LC_MESSAGES/'))
	NSIINPUTFILE = $(WINDOWSSTAGING)/$(NAME).nsi.in
	NSIFILE = $(WINDOWSSTAGING)/$(NAME).nsi
	MAKENSIS = makensis
	XSLTDIR = .\\xslt
	TARGET = $(NAME).exe
endif

LIBS = $(LIBQT) $(LIBXML2) $(LIBXSLT) $(LIBSQLITE3) $(LIBDIVECOMPUTER) \
	$(EXTRALIBS) $(LIBZIP) -lpthread -lm $(LIBSOUP) $(LIBWINSOCK) $(MARBLELIBS)

MSGLANGS=$(notdir $(wildcard po/*.po))

# Add files to the following variables if the auto-detection based on the
# filename fails
OBJS_NEEDING_MOC =
HEADERS_NEEDING_MOC =

include Rules.mk
