include(subsurface-configure.pri)

QT = core gui network webkit svg
INCLUDEPATH += qt-ui $$PWD

VERSION = 3.1

HEADERS = \
	color.h \
	deco.h \
	device.h \
	display.h \
	dive.h \
	divelist.h \
	file.h \
	flag.h \
	gettextfromc.h \
	gettext.h \
	helpers.h \
	libdivecomputer.h \
	planner.h \
	pref.h \
	profile.h \
	qt-gui.h \
	qthelper.h \
	qt-ui/about.h \
	qt-ui/completionmodels.h \
	qt-ui/divecomputermanagementdialog.h \
	qt-ui/divelistview.h \
	qt-ui/diveplanner.h \
	qt-ui/downloadfromdivecomputer.h \
	qt-ui/globe.h \
	qt-ui/graphicsview-common.h \
	qt-ui/kmessagewidget.h \
	qt-ui/maintab.h \
	qt-ui/mainwindow.h \
	qt-ui/modeldelegates.h \
	qt-ui/models.h \
	qt-ui/plotareascene.h \
	qt-ui/preferences.h \
	qt-ui/printdialog.h \
	qt-ui/printlayout.h \
	qt-ui/printoptions.h \
	qt-ui/profilegraphics.h \
	qt-ui/simplewidgets.h \
	qt-ui/starwidget.h \
	qt-ui/subsurfacewebservices.h \
	qt-ui/tableview.h \
	satellite.h \
	sha1.h \
	statistics.h \
	subsurface-icon.h \
	subsurfacestartup.h \
	uemis.h \
	webservice.h 

SOURCES =  \
	deco.c \
	device.c \
	dive.c \
	divelist.c \
	equipment.c \
	file.c \
	gettextfromc.cpp \
	libdivecomputer.c \
	main.cpp \
	parse-xml.c \
	planner.c \
	profile.c \
	qt-gui.cpp \
	qthelper.cpp \
	qt-ui/about.cpp \
	qt-ui/completionmodels.cpp \
	qt-ui/divecomputermanagementdialog.cpp \
	qt-ui/divelistview.cpp \
	qt-ui/diveplanner.cpp \
	qt-ui/downloadfromdivecomputer.cpp \
	qt-ui/globe.cpp \
	qt-ui/graphicsview-common.cpp \
	qt-ui/kmessagewidget.cpp \
	qt-ui/maintab.cpp \
	qt-ui/mainwindow.cpp \
	qt-ui/modeldelegates.cpp \
	qt-ui/models.cpp \
	qt-ui/plotareascene.cpp \
	qt-ui/preferences.cpp \
	qt-ui/printdialog.cpp \
	qt-ui/printlayout.cpp \
	qt-ui/printoptions.cpp \
	qt-ui/profilegraphics.cpp \
	qt-ui/simplewidgets.cpp \
	qt-ui/starwidget.cpp \
	qt-ui/subsurfacewebservices.cpp \
	qt-ui/tableview.cpp \
	save-xml.c \
	sha1.c \
	statistics.c \
	subsurfacestartup.c \
	time.c \
	uemis.c \
	uemis-downloader.c

linux*: SOURCES += linux.c
mac: SOURCES += macos.c
win32: SOURCES += windows.c

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

RESOURCES = subsurface.qrc

TRANSLATIONS = subsurface_de.ts 

doc.commands = $(CHK_DIR_EXISTS) Documentation || $(MKDIR) Documentation
doc.commands += $$escape_expand(\\n\\t)$(MAKE) -C $$PWD/Documentation OUT=$$OUT_PWD/Documentation doc
all.depends += doc
QMAKE_EXTRA_TARGETS += doc all

include(subsurface-gen-version.pri)
