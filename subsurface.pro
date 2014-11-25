CODECFORTR = UTF-8
CODECFORSRC = UTF-8
include(subsurface-configure.pri)

QT = core gui network svg
lessThan(QT_MAJOR_VERSION, 5) {
	QT += webkit
} else {
	QT += printsupport concurrent
	!android: QT += webkitwidgets webkit
	android: QT += androidextras
}
INCLUDEPATH += qt-ui $$PWD
DEPENDPATH += qt-ui

mac: TARGET = Subsurface
else: TARGET = subsurface

QMAKE_CLEAN += $$TARGET

VERSION = 4.2.90

HEADERS = \
	cochran.h \
	color.h \
	deco.h \
	device.h \
	display.h \
	dive.h \
	divelist.h \
	file.h \
	gettextfromc.h \
	gettext.h \
	helpers.h \
	libdivecomputer.h \
	planner.h \
	save-html.h \
	worldmap-save.h \
	worldmap-options.h \
	pref.h \
	profile.h \
	gaspressures.h \
	qt-gui.h \
	qthelper.h \
	units.h \
	divecomputer.h \
	qt-ui/about.h \
	qt-ui/completionmodels.h \
	qt-ui/divecomputermanagementdialog.h \
	qt-ui/divelistview.h \
	qt-ui/divepicturewidget.h \
	qt-ui/diveplanner.h \
	qt-ui/downloadfromdivecomputer.h \
	qt-ui/globe.h \
	qt-ui/graphicsview-common.h \
	qt-ui/kmessagewidget.h \
	qt-ui/maintab.h \
	qt-ui/mainwindow.h \
	qt-ui/modeldelegates.h \
	qt-ui/models.h \
	qt-ui/metrics.h \
	qt-ui/preferences.h \
	qt-ui/printdialog.h \
	qt-ui/printlayout.h \
	qt-ui/printoptions.h \
	qt-ui/simplewidgets.h \
	qt-ui/starwidget.h \
	qt-ui/subsurfacewebservices.h \
	qt-ui/tableview.h \
	exif.h \
	sha1.h \
	statistics.h \
	subsurfacestartup.h \
	uemis.h \
	webservice.h \
	qt-ui/divelogimportdialog.h \
	qt-ui/tagwidget.h \
	qt-ui/groupedlineedit.h \
	qt-ui/usermanual.h \
	qt-ui/profile/profilewidget2.h \
	qt-ui/profile/diverectitem.h \
	qt-ui/profile/divepixmapitem.h \
	qt-ui/profile/divelineitem.h \
	qt-ui/profile/divetextitem.h \
	qt-ui/profile/animationfunctions.h \
	qt-ui/profile/divecartesianaxis.h \
	qt-ui/profile/diveplotdatamodel.h \
	qt-ui/profile/diveprofileitem.h \
	qt-ui/profile/diveeventitem.h \
	qt-ui/profile/divetooltipitem.h \
	qt-ui/profile/ruleritem.h \
	qt-ui/profile/tankitem.h \
	qt-ui/updatemanager.h \
	qt-ui/divelogexportdialog.h \
	qt-ui/usersurvey.h \
	subsurfacesysinfo.h \
	qt-ui/configuredivecomputerdialog.h \
	configuredivecomputer.h \
	configuredivecomputerthreads.h \
	devicedetails.h \
	qt-ui/statistics/monthstatistics.h \
	qt-ui/statistics/statisticswidget.h \
	qt-ui/statistics/statisticsbar.h \
	qt-ui/statistics/yearstatistics.h \
	qt-ui/diveshareexportdialog.h \
	qt-ui/filtermodels.h

android: HEADERS -= \
	qt-ui/usermanual.h \
	qt-ui/printdialog.h \
	qt-ui/printlayout.h \
	qt-ui/printoptions.h

SOURCES =  \
	cochran.c \
	deco.c \
	device.c \
	dive.c \
	divelist.c \
	equipment.c \
	file.c \
	gettextfromc.cpp \
	libdivecomputer.c \
	liquivision.c \
	load-git.c \
	main.cpp \
	membuffer.c \
	parse-xml.c \
	planner.c \
	profile.c \
	gaspressures.c \
	divecomputer.cpp \
	worldmap-save.c \
	save-html.c \
	qt-gui.cpp \
	qthelper.cpp \
	qt-ui/about.cpp \
	qt-ui/completionmodels.cpp \
	qt-ui/divecomputermanagementdialog.cpp \
	qt-ui/divelistview.cpp \
	qt-ui/divepicturewidget.cpp \
	qt-ui/diveplanner.cpp \
	qt-ui/downloadfromdivecomputer.cpp \
	qt-ui/globe.cpp \
	qt-ui/graphicsview-common.cpp \
	qt-ui/kmessagewidget.cpp \
	qt-ui/maintab.cpp \
	qt-ui/mainwindow.cpp \
	qt-ui/modeldelegates.cpp \
	qt-ui/models.cpp \
	qt-ui/metrics.cpp \
	qt-ui/preferences.cpp \
	qt-ui/printdialog.cpp \
	qt-ui/printlayout.cpp \
	qt-ui/printoptions.cpp \
	qt-ui/simplewidgets.cpp \
	qt-ui/starwidget.cpp \
	qt-ui/subsurfacewebservices.cpp \
	qt-ui/tableview.cpp \
	exif.cpp \
	save-git.c \
	save-xml.c \
	sha1.c \
	statistics.c \
	strtod.c \
	subsurfacestartup.c \
	time.c \
	uemis.c \
	uemis-downloader.c \
	qt-ui/divelogimportdialog.cpp \
	qt-ui/tagwidget.cpp \
	qt-ui/groupedlineedit.cpp \
	qt-ui/usermanual.cpp \
	qt-ui/profile/profilewidget2.cpp \
	qt-ui/profile/diverectitem.cpp \
	qt-ui/profile/divepixmapitem.cpp \
	qt-ui/profile/divelineitem.cpp \
	qt-ui/profile/divetextitem.cpp \
	qt-ui/profile/animationfunctions.cpp \
	qt-ui/profile/divecartesianaxis.cpp \
	qt-ui/profile/diveplotdatamodel.cpp \
	qt-ui/profile/diveprofileitem.cpp \
	qt-ui/profile/diveeventitem.cpp \
	qt-ui/profile/divetooltipitem.cpp \
	qt-ui/profile/ruleritem.cpp \
	qt-ui/profile/tankitem.cpp \
	qt-ui/updatemanager.cpp \
	qt-ui/divelogexportdialog.cpp \
	qt-ui/usersurvey.cpp \
	subsurfacesysinfo.cpp \
	qt-ui/configuredivecomputerdialog.cpp \
	configuredivecomputer.cpp \
	configuredivecomputerthreads.cpp \
	devicedetails.cpp \
	qt-ui/statistics/statisticswidget.cpp \
	qt-ui/statistics/yearstatistics.cpp \
	qt-ui/statistics/statisticsbar.cpp \
	qt-ui/statistics/monthstatistics.cpp \
	qt-ui/diveshareexportdialog.cpp \
	qt-ui/filtermodels.cpp

android: SOURCES += android.cpp
else: win32: SOURCES += windows.c
else: mac: SOURCES += macos.c
else: SOURCES += linux.c        # All other Unix, really

android: SOURCES -= \
	qt-ui/usermanual.cpp \
	qt-ui/printdialog.cpp \
	qt-ui/printlayout.cpp \
	qt-ui/printoptions.cpp

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
	qt-ui/setpoint.ui \
	qt-ui/shifttimes.ui \
	qt-ui/shiftimagetimes.ui \
	qt-ui/webservices.ui \
	qt-ui/tableview.ui \
	qt-ui/divelogimportdialog.ui \
	qt-ui/searchbar.ui \
	qt-ui/divelogexportdialog.ui \
	qt-ui/plannerSettings.ui \
	qt-ui/usersurvey.ui \
	qt-ui/divecomponentselection.ui \
	qt-ui/configuredivecomputerdialog.ui \
	qt-ui/listfilter.ui \
	qt-ui/diveshareexportdialog.ui \
	qt-ui/filterwidget.ui

# Nether usermanual or printing is supported on android right now
android: FORMS -= qt-ui/printoptions.ui

RESOURCES = subsurface.qrc

TRANSLATIONS = \
	translations/subsurface_source.ts \
	translations/subsurface_bg_BG.ts \
	translations/subsurface_da_DK.ts \
	translations/subsurface_de_CH.ts \
	translations/subsurface_de_DE.ts \
	translations/subsurface_el_GR.ts \
	translations/subsurface_en_GB.ts \
	translations/subsurface_es_ES.ts \
	translations/subsurface_et_EE.ts \
	translations/subsurface_fi_FI.ts \
	translations/subsurface_fr_FR.ts \
	translations/subsurface_he.ts \
	translations/subsurface_hu.ts \
	translations/subsurface_it_IT.ts \
	translations/subsurface_lv_LV.ts \
	translations/subsurface_nb_NO.ts \
	translations/subsurface_nl_NL.ts \
	translations/subsurface_pl_PL.ts \
	translations/subsurface_pt_BR.ts \
	translations/subsurface_pt_PT.ts \
	translations/subsurface_ro_RO.ts \
	translations/subsurface_ru_RU.ts \
	translations/subsurface_sk_SK.ts \
	translations/subsurface_sv_SE.ts \
	translations/subsurface_tr.ts \
	translations/subsurface_zh_TW.ts

QTTRANSLATIONS = \
	qt_da.qm \
	qt_de.qm \
	qt_es.qm \
	qt_fr.qm \
	qt_he.qm \
	qt_hu.qm \
	qt_pl.qm \
	qt_pt.qm \
	qt_ru.qm \
	qt_sk.qm \
	qt_sv.qm \
	qt_zh_TW.qm

greaterThan(QT_MAJOR_VERSION, 4) {
QTRANSLATIONS += \
	qtbase_de.qm \
	qt_fi.qm \
	qtbase_fi.qm \
	qtbase_hu.qm \
	qtbase_ru.qm \
	qtbase_sk.qm
}

USERMANUALS = \
	user-manual.html \
	user-manual_es.html \
	user-manual_ru.html

doc.commands += $(CHK_DIR_EXISTS) $$OUT_PWD/Documentation || $(MKDIR) $$OUT_PWD/Documentation $$escape_expand(\\n\\t)$(MAKE) -C $$PWD/Documentation OUT=$$OUT_PWD/Documentation/ doc
all.depends += usermanual
usermanual.depends += doc
usermanual.target = $$OUT_PWD/Documentation/user-manual.html
QMAKE_EXTRA_TARGETS += doc usermanual all
# add the generated user manual HTML files to the list of files to remove
# when running make clean
for(MANUAL,USERMANUALS) QMAKE_CLEAN += $$OUT_PWD/Documentation/$$MANUAL

marbledata.commands += $(CHK_DIR_EXISTS) $$OUT_PWD/marbledata || $(COPY_DIR) $$PWD/marbledata $$OUT_PWD
all.depends += marbledata
QMAKE_EXTRA_TARGETS += marbledata

theme.commands += $(CHK_DIR_EXISTS) $$OUT_PWD/theme || $(COPY_DIR) $$PWD/theme $$OUT_PWD
all.depends += theme
QMAKE_EXTRA_TARGETS += theme

android {
	android.commands += $(CHK_DIR_EXISTS) $$OUT_PWD/android || $(COPY_DIR) $$PWD/android $$OUT_PWD
	all.depends += android
	QMAKE_EXTRA_TARGETS += android
}

DESKTOP_FILE = subsurface.desktop
mac: ICON = packaging/macosx/Subsurface.icns
else: ICON = subsurface-icon.svg
MANPAGE = subsurface.1
XSLT_FILES = xslt
ICONS_FILES = icons
DOC_FILES = Documentation/images README Releasenotes.txt SupportedDivecomputers.txt
for(MANUAL,USERMANUALS) DOC_FILES += $$OUT_PWD/Documentation/$$MANUAL
THEME_FILES = theme
MARBLEDIR = marbledata/maps marbledata/bitmaps

#DEPLOYMENT_PLUGIN += bearer/qnativewifibearer
DEPLOYMENT_PLUGIN += codecs/qcncodecs codecs/qjpcodecs codecs/qkrcodecs codecs/qtwcodecs
DEPLOYMENT_PLUGIN += imageformats/qgif imageformats/qjpeg imageformats/qsvg
DEPLOYMENT_PLUGIN += iconengines/qsvgicon
#DEPLOYMENT_PLUGIN += sqldrivers/qsqlite

# This information will go into the Windows .rc file and linked into the .exe
QMAKE_TARGET_COMPANY = Subsurface Team
QMAKE_TARGET_DESCRIPTION = Subsurface Dive Log
QMAKE_TARGET_COPYRIGHT = Linus Torvalds, Dirk Hohndel, Tomaz Canabrava and others

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

# to debug planner issues
#QMAKE_CFLAGS += -DDEBUG_PLAN=31
#QMAKE_CXXFLAGS += -DDEBUG_PLAN=31
# to build debuggable binaries on Windows, you need something like this
#QMAKE_CFLAGS_RELEASE=$$QMAKE_CFLAGS_DEBUG -O0 -g
#QMAKE_CXXFLAGS_RELEASE=$$QMAKE_CXXFLAGS_DEBUG -O0 -g

QMAKE_CXXFLAGS += $$(CXXFLAGS)
QMAKE_CFLAGS += $$(CFLAGS)
QMAKE_LFLAGS += $$(LDFLAGS)
QMAKE_CPPFLAGS += $$(CPPFLAGS)
