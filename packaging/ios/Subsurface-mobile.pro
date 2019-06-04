TEMPLATE = app

QT += qml quick quickcontrols2 widgets positioning concurrent svg bluetooth

DEFINES += SUBSURFACE_MOBILE BT_SUPPORT BLE_SUPPORT

CONFIG += c++11

SOURCES += ../../subsurface-mobile-main.cpp \
	../../subsurface-helper.cpp \
	../../map-widget/qmlmapwidgethelper.cpp \
	../../core/cloudstorage.cpp \
	../../core/configuredivecomputerthreads.cpp \
	../../core/devicedetails.cpp \
	../../core/gpslocation.cpp \
	../../core/imagedownloader.cpp \
	../../core/downloadfromdcthread.cpp \
	../../core/qtserialbluetooth.cpp \
	../../core/plannernotes.c \
	../../core/uemis-downloader.c \
	../../core/applicationstate.cpp \
	../../core/qthelper.cpp \
	../../core/checkcloudconnection.cpp \
	../../core/color.cpp \
	../../core/configuredivecomputer.cpp \
	../../core/divecomputer.cpp \
	../../core/divelogexportlogic.cpp \
	../../core/divesitehelpers.cpp \
	../../core/errorhelper.c \
	../../core/exif.cpp \
	../../core/format.cpp \
	../../core/gettextfromc.cpp \
	../../core/metrics.cpp \
	../../core/qt-init.cpp \
	../../core/subsurfacesysinfo.cpp \
	../../core/windowtitleupdate.cpp \
	../../core/file.c \
	../../core/subsurfacestartup.c \
	../../core/ios.cpp \
	../../core/profile.c \
	../../core/device.c \
	../../core/dive.c \
	../../core/divelist.c \
	../../core/gas-model.c \
	../../core/gaspressures.c \
	../../core/git-access.c \
	../../core/liquivision.c \
	../../core/load-git.c \
	../../core/parse-xml.c \
	../../core/parse.c \
	../../core/import-suunto.c \
	../../core/import-shearwater.c \
	../../core/import-cobalt.c \
	../../core/import-divinglog.c \
	../../core/import-csv.c \
	../../core/save-html.c \
	../../core/statistics.c \
	../../core/worldmap-save.c \
	../../core/libdivecomputer.c \
	../../core/version.c \
	../../core/save-git.c \
	../../core/datatrak.c \
	../../core/ostctools.c \
	../../core/planner.c \
	../../core/save-xml.c \
	../../core/cochran.c \
	../../core/deco.c \
	../../core/divesite.c \
	../../core/equipment.c \
	../../core/gas.c \
	../../core/membuffer.c \
	../../core/sha1.c \
	../../core/strtod.c \
	../../core/tag.c \
	../../core/taxonomy.c \
	../../core/time.c \
	../../core/trip.c \
	../../core/units.c \
	../../core/uemis.c \
	../../core/btdiscovery.cpp \
	../../core/connectionlistmodel.cpp \
	../../core/qt-ble.cpp \
	../../core/settings/qPref.cpp \
	../../core/settings/qPrefCloudStorage.cpp \
	../../core/settings/qPrefDisplay.cpp \
	../../core/settings/qPrefDiveComputer.cpp \
	../../core/settings/qPrefDivePlanner.cpp \
	../../core/settings/qPrefGeneral.cpp \
	../../core/settings/qPrefGeocoding.cpp \
	../../core/settings/qPrefLanguage.cpp \
	../../core/settings/qPrefLocationService.cpp \
	../../core/settings/qPrefPartialPressureGas.cpp \
	../../core/settings/qPrefPrivate.cpp \
	../../core/settings/qPrefProxy.cpp \
	../../core/settings/qPrefTechnicalDetails.cpp \
	../../core/settings/qPrefUnit.cpp \
	../../core/settings/qPrefUpdateManager.cpp \
	../../core/subsurface-qt/CylinderObjectHelper.cpp \
	../../core/subsurface-qt/DiveObjectHelper.cpp \
	../../core/subsurface-qt/DiveListNotifier.cpp \
	../../mobile-widgets/qmlmanager.cpp \
	../../mobile-widgets/qmlprefs.cpp \
	../../qt-models/divelistmodel.cpp \
	../../qt-models/diveplotdatamodel.cpp \
	../../qt-models/gpslistmodel.cpp \
	../../qt-models/completionmodels.cpp \
	../../qt-models/divelocationmodel.cpp \
	../../qt-models/maplocationmodel.cpp \
	../../qt-models/diveimportedmodel.cpp \
	../../qt-models/messagehandlermodel.cpp \
	../../profile-widget/qmlprofile.cpp \
	../../profile-widget/divecartesianaxis.cpp \
	../../profile-widget/diveeventitem.cpp \
	../../profile-widget/diveprofileitem.cpp \
	../../profile-widget/profilewidget2.cpp \
	../../profile-widget/ruleritem.cpp \
	../../profile-widget/animationfunctions.cpp \
	../../profile-widget/divepixmapitem.cpp \
	../../profile-widget/divetooltipitem.cpp \
	../../profile-widget/tankitem.cpp \
	../../profile-widget/divelineitem.cpp \
	../../profile-widget/diverectitem.cpp \
	../../profile-widget/divetextitem.cpp

RESOURCES += ./qml.qrc \
			../../mobile-widgets/qml/mobile-resources.qrc \
			../../map-widget/qml/map-widget.qrc \
			./translations.qrc

LIBS += ./install-root/lib/libdivecomputer.a \
	./install-root/lib/libgit2.a \
	./install-root/lib/libzip.a \
	./install-root/lib/libxslt.a \
	./build-ios/googlemaps-build/libqtgeoservices_googlemaps.a \
	-liconv \
	-lsqlite3 \
	-lxml2

INCLUDEPATH += ./install-root/include/ \
	./install-root/lib/libzip/include \
	./install-root/include/libxstl \
	./install-root/include/libexstl \
	./install-root/include/openssl \
	../.. \
	../../core \
	../../mobile-widgets/qml/kirigami/src/libkirigami \
	/usr/include/libxml2

HEADERS += \
	../../core/libdivecomputer.h \
	../../core/cloudstorage.h \
	../../core/configuredivecomputerthreads.h \
	../../core/device.h \
	../../core/devicedetails.h \
	../../core/dive.h \
	../../core/git-access.h \
	../../core/gpslocation.h \
	../../core/imagedownloader.h \
	../../core/pref.h \
	../../core/profile.h \
	../../core/qthelper.h \
	../../core/save-html.h \
	../../core/statistics.h \
	../../core/units.h \
	../../core/version.h \
	../../core/planner.h \
	../../core/divesite.h \
	../../core/checkcloudconnection.h \
	../../core/cochran.h \
	../../core/color.h \
	../../core/configuredivecomputer.h \
	../../core/datatrak.h \
	../../core/deco.h \
	../../core/display.h \
	../../core/divecomputer.h \
	../../core/divelist.h \
	../../core/divelogexportlogic.h \
	../../core/divesitehelpers.h \
	../../core/exif.h \
	../../core/file.h \
	../../core/gaspressures.h \
	../../core/gettext.h \
	../../core/gettextfromc.h \
	../../core/membuffer.h \
	../../core/metrics.h \
	../../core/qt-gui.h \
	../../core/sha1.h \
	../../core/strndup.h \
	../../core/subsurfacestartup.h \
	../../core/subsurfacesysinfo.h \
	../../core/taxonomy.h \
	../../core/uemis.h \
	../../core/webservice.h \
	../../core/windowtitleupdate.h \
	../../core/worldmap-options.h \
	../../core/worldmap-save.h \
	../../core/downloadfromdcthread.h \
	../../core/btdiscovery.h \
	../../core/connectionlistmodel.h \
	../../core/qt-ble.h \
	../../core/settings/qPref.h \
	../../core/settings/qPrefCloudStorage.h \
	../../core/settings/qPrefDisplay.h \
	../../core/settings/qPrefDiveComputer.h \
	../../core/settings/qPrefDivePlanner.h \
	../../core/settings/qPrefGeneral.h \
	../../core/settings/qPrefGeocoding.h \
	../../core/settings/qPrefLanguage.h \
	../../core/settings/qPrefLocationService.h \
	../../core/settings/qPrefPartialPressureGas.h \
	../../core/settings/qPrefPrivate.h \
	../../core/settings/qPrefProxy.h \
	../../core/settings/qPrefTechnicalDetails.h \
	../../core/settings/qPrefUnit.h \
	../../core/settings/qPrefUpdateManager.h \
	../../core/subsurface-qt/CylinderObjectHelper.h \
	../../core/subsurface-qt/DiveObjectHelper.h \
	../../core/subsurface-qt/DiveListNotifier.h \
	../../mobile-widgets/qmlmanager.h \
	../../mobile-widgets/qmlprefs.h \
	../../map-widget/qmlmapwidgethelper.h \
	../../qt-models/divelistmodel.h \
	../../qt-models/diveplotdatamodel.h \
	../../qt-models/gpslistmodel.h \
	../../qt-models/divelocationmodel.h \
	../../qt-models/completionmodels.h \
	../../qt-models/weightsysteminfomodel.h \
	../../qt-models/maplocationmodel.h \
	../../qt-models/diveimportedmodel.h \
	../../qt-models/messagehandlermodel.h \
	../../profile-widget/qmlprofile.h \
	../../profile-widget/diveprofileitem.h \
	../../profile-widget/profilewidget2.h \
	../../profile-widget/ruleritem.h \
	../../profile-widget/diveeventitem.h \
	../../profile-widget/divetooltipitem.h \
	../../profile-widget/tankitem.h \
	../../profile-widget/animationfunctions.h \
	../../profile-widget/divecartesianaxis.h \
	../../profile-widget/divelineitem.h \
	../../profile-widget/divepixmapitem.h \
	../../profile-widget/diverectitem.h \
	../../profile-widget/divetextitem.h

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(./deployment.pri)

include(../../mobile-widgets/qml/kirigami/kirigami.pri)

# Supress some warnings
QMAKE_CXXFLAGS += -Wno-shorten-64-to-32 -Wno-missing-field-initializers
QMAKE_CFLAGS   += -Wno-shorten-64-to-32 -Wno-missing-field-initializers

ios {
	QMAKE_ASSET_CATALOGS += ./storeIcon.xcassets
	app_launch_images.files = ./SubsurfaceMobileLaunch.xib $$files(./SubsurfaceMobileLaunchImage*.png)
	images.files = ../../icons/subsurface-mobile-icon.png
	QMAKE_BUNDLE_DATA += app_launch_images images
	QMAKE_INFO_PLIST = ./Info.plist
	QMAKE_IOS_DEPLOYMENT_TARGET = 10.0
}
