include(subsurface-configure.pri)

QT = core gui network webkit svg
INCLUDEPATH += qt-ui $$PWD

mac: TARGET = Subsurface
else: TARGET = subsurface

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
	webservice.h \
	qt-ui/csvimportdialog.h \
	qt-ui/tagwidget.h \
	qt-ui/groupedlineedit.h

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
	uemis-downloader.c \
	qt-ui/csvimportdialog.cpp \
	qt-ui/tagwidget.cpp \
	qt-ui/groupedlineedit.cpp

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
	qt-ui/webservices.ui \
	qt-ui/tableview.ui \
	qt-ui/csvimportdialog.ui

RESOURCES = subsurface.qrc

TRANSLATIONS = \
	translations/subsurface_source.ts \
	translations/subsurface_bg_BG.ts \
	translations/subsurface_ca_ES.ts \
	translations/subsurface_da_DK.ts \
	translations/subsurface_de_CH.ts \
	translations/subsurface_de_DE.ts \
	translations/subsurface_es_ES.ts \
	translations/subsurface_et_EE.ts \
	translations/subsurface_fi_FI.ts \
	translations/subsurface_fr_FR.ts \
	translations/subsurface_hr_HR.ts \
	translations/subsurface_it_IT.ts \
	translations/subsurface_nb_NO.ts \
	translations/subsurface_nl_NL.ts \
	translations/subsurface_pl_PL.ts \
	translations/subsurface_pt_BR.ts \
	translations/subsurface_pt_PT.ts \
	translations/subsurface_ru_RU.ts \
	translations/subsurface_sk_SK.ts \
	translations/subsurface_sv_SE.ts

QTTRANSLATIONS = \
	qt_da.qm \
	qt_de.qm \
	qt_es.qm \
	qt_fr.qm \
	qt_pl.qm \
	qt_pt.qm \
	qt_ru.qm \
	qt_sk.qm \
	qt_sv.qm

doc.commands += $$escape_expand(\\n\\t)$(MAKE) -C $$PWD/Documentation OUT=$$OUT_PWD/Documentation/ doc
all.depends += doc
QMAKE_EXTRA_TARGETS += doc all

DESKTOP_FILE = subsurface.desktop
mac: ICON = packaging/macosx/Subsurface.icns
else: ICON = subsurface-icon.svg
MANPAGE = subsurface.1
XSLT_FILES = xslt
DOC_FILES = $$OUT_PWD/Documentation/user-manual.html Documentation/images
MARBLEDIR = marbledata/maps marbledata/bitmaps
DEPLOYMENT_PLUGIN += imageformats/qjpeg

# This information will go into the Windows .rc file and linked into the .exe
QMAKE_TARGET_COMPANY = subsurface team
QMAKE_TARGET_DESCRIPTION = subsurface dive log
QMAKE_TARGET_COPYRIGHT = Linus Torvalds, Dirk Hohndel and others

# And this is the Mac Info.plist file
# qmake automatically generates sed rules to replace:
#  token                qmake expansion
#  @ICON@               $$ICON
#  @TYPEINFO@           first 4 chars of $$QMAKE_PKGINFO_TYPEINFO
#  @EXECUTABLE@         $$QMAKE_ORIG_TARGET
#  @LIBRARY@            $$QMAKE_ORIG_TARGET
#  @SHORT_VERSION@      $$VER_MAJ.$$VER_MIN
QMAKE_INFO_PLIST = packaging/macosx/Info.plist.in

OTHER_FILES += $$DESKTOPFILE $$ICON $$MANPAGE $$XSLT_FILES $$DOC_FILES $$MARBLEDIR \
        $$QMAKE_INFO_PLIST

include(subsurface-gen-version.pri)
include(subsurface-install.pri)
