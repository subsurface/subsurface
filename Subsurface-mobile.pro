TEMPLATE = app

QT += qml quick quickcontrols2 widgets positioning concurrent svg bluetooth 

DEFINES += SUBSURFACE_MOBILE BT_SUPPORT BLE_SUPPORT

CONFIG += c++17
CONFIG += qtquickcompiler

SOURCES += subsurface-mobile-main.cpp \
	subsurface-helper.cpp \
	map-widget/qmlmapwidgethelper.cpp \
	commands/command_base.cpp \
	commands/command.cpp \
	commands/command_device.cpp \
	commands/command_divelist.cpp \
	commands/command_divesite.cpp \
	commands/command_edit.cpp \
	commands/command_edit_trip.cpp \
	commands/command_event.cpp \
	commands/command_filter.cpp \
	commands/command_pictures.cpp \
	core/cloudstorage.cpp \
	core/configuredivecomputerthreads.cpp \
	core/devicedetails.cpp \
	core/gpslocation.cpp \
	core/downloadfromdcthread.cpp \
	core/qtserialbluetooth.cpp \
	core/plannernotes.c \
	core/uemis-downloader.c \
	core/qthelper.cpp \
	core/checkcloudconnection.cpp \
	core/color.cpp \
	core/configuredivecomputer.cpp \
	core/divelogexportlogic.cpp \
	core/divesitehelpers.cpp \
	core/errorhelper.c \
	core/exif.cpp \
	core/format.cpp \
	core/gettextfromc.cpp \
	core/metrics.cpp \
	core/qt-init.cpp \
	core/subsurfacesysinfo.cpp \
	core/windowtitleupdate.cpp \
	core/file.c \
	core/fulltext.cpp \
	core/subsurfacestartup.c \
	core/pref.c \
	core/profile.c \
	core/device.cpp \
	core/dive.c \
	core/divecomputer.c \
	core/divefilter.cpp \
	core/event.c \
	core/filterconstraint.cpp \
	core/filterpreset.cpp \
	core/divelist.c \
	core/gas-model.c \
	core/gaspressures.c \
	core/git-access.c \
	core/liquivision.c \
	core/load-git.c \
	core/parse-xml.c \
	core/parse.c \
	core/picture.c \
	core/pictureobj.cpp \
	core/sample.c \
	core/import-suunto.c \
	core/import-shearwater.c \
	core/import-seac.c \
	core/import-cobalt.c \
	core/import-divinglog.c \
	core/import-csv.c \
	core/save-html.c \
	core/statistics.c \
	core/worldmap-save.c \
	core/libdivecomputer.c \
	core/version.c \
	core/save-git.c \
	core/datatrak.c \
	core/ostctools.c \
	core/planner.c \
	core/save-xml.c \
	core/cochran.c \
	core/deco.c \
	core/divesite.c \
	core/equipment.c \
	core/gas.c \
	core/membuffer.c \
	core/selection.cpp \
	core/sha1.c \
	core/string-format.cpp \
	core/strtod.c \
	core/tag.c \
	core/taxonomy.c \
	core/time.c \
	core/trip.c \
	core/units.c \
	core/uemis.c \
	core/btdiscovery.cpp \
	core/connectionlistmodel.cpp \
	core/qt-ble.cpp \
	core/uploadDiveShare.cpp \
	core/uploadDiveLogsDE.cpp \
	core/save-profiledata.c \
	core/xmlparams.cpp \
	core/settings/qPref.cpp \
	core/settings/qPrefCloudStorage.cpp \
	core/settings/qPrefDisplay.cpp \
	core/settings/qPrefDiveComputer.cpp \
	core/settings/qPrefDivePlanner.cpp \
	core/settings/qPrefGeneral.cpp \
	core/settings/qPrefGeocoding.cpp \
	core/settings/qPrefLanguage.cpp \
	core/settings/qPrefLocationService.cpp \
	core/settings/qPrefPartialPressureGas.cpp \
	core/settings/qPrefPrivate.cpp \
	core/settings/qPrefProxy.cpp \
	core/settings/qPrefTechnicalDetails.cpp \
	core/settings/qPrefUnit.cpp \
	core/settings/qPrefEquipment.cpp \
	core/settings/qPrefLog.cpp \
	core/settings/qPrefMedia.cpp \
	core/settings/qPrefUpdateManager.cpp \
	core/subsurface-qt/divelistnotifier.cpp \
	backend-shared/exportfuncs.cpp \
	backend-shared/plannershared.cpp \
	backend-shared/roundrectitem.cpp \
	stats/statsvariables.cpp \
	stats/statsview.cpp \
	stats/barseries.cpp \
	stats/boxseries.cpp \
	stats/chartitem.cpp \
	stats/chartlistmodel.cpp \
	stats/histogrammarker.cpp \
	stats/informationbox.cpp \
	stats/legend.cpp \
	stats/pieseries.cpp \
	stats/quartilemarker.cpp \
	stats/regressionitem.cpp \
	stats/scatterseries.cpp \
	stats/statsaxis.cpp \
	stats/statscolors.cpp \
	stats/statsgrid.cpp \
	stats/statshelper.cpp \
	stats/statsselection.cpp \
	stats/statsseries.cpp \
	stats/statsstate.cpp \
	mobile-widgets/qmlinterface.cpp \
	mobile-widgets/qmlmanager.cpp \
	mobile-widgets/statsmanager.cpp \
	mobile-widgets/themeinterface.cpp \
	qt-models/divesummarymodel.cpp \
	qt-models/diveplotdatamodel.cpp \
	qt-models/gpslistmodel.cpp \
	qt-models/completionmodels.cpp \
	qt-models/divelocationmodel.cpp \
	qt-models/maplocationmodel.cpp \
	qt-models/diveimportedmodel.cpp \
	qt-models/messagehandlermodel.cpp \
	qt-models/diveplannermodel.cpp \
	qt-models/divetripmodel.cpp \
	qt-models/mobilelistmodel.cpp \
	qt-models/cylindermodel.cpp \
	qt-models/cleanertablemodel.cpp \
	qt-models/tankinfomodel.cpp \
	qt-models/models.cpp \
	qt-models/weightsysteminfomodel.cpp \
	qt-models/filterconstraintmodel.cpp \
	qt-models/filterpresetmodel.cpp \
	profile-widget/qmlprofile.cpp \
	profile-widget/divecartesianaxis.cpp \
	profile-widget/diveeventitem.cpp \
	profile-widget/diveprofileitem.cpp \
	profile-widget/profilewidget2.cpp \
	profile-widget/ruleritem.cpp \
	profile-widget/animationfunctions.cpp \
	profile-widget/divepixmapitem.cpp \
	profile-widget/divetooltipitem.cpp \
	profile-widget/tankitem.cpp \
	profile-widget/divehandler.cpp \
	profile-widget/divelineitem.cpp \
	profile-widget/diverectitem.cpp \
	profile-widget/divetextitem.cpp

HEADERS += \
	commands/command_base.h \
	commands/command.h \
	commands/command_device.h \
	commands/command_divelist.h \
	commands/command_divesite.h \
	commands/command_edit.h \
	commands/command_edit_trip.h \
	commands/command_event.h \
	commands/command_filter.h \
	commands/command_pictures.h \
	core/interpolate.h \
	core/libdivecomputer.h \
	core/cloudstorage.h \
	core/configuredivecomputerthreads.h \
	core/device.h \
	core/devicedetails.h \
	core/dive.h \
	core/divecomputer.h \
	core/event.h \
	core/extradata.h \
	core/git-access.h \
	core/gpslocation.h \
	core/pref.h \
	core/profile.h \
	core/qthelper.h \
	core/save-html.h \
	core/statistics.h \
	core/units.h \
	core/version.h \
	core/picture.h \
	core/pictureobj.h \
	core/planner.h \
	core/divesite.h \
	core/checkcloudconnection.h \
	core/cochran.h \
	core/color.h \
	core/configuredivecomputer.h \
	core/datatrak.h \
	core/deco.h \
	core/display.h \
	core/divefilter.h \
	core/filterconstraint.h \
	core/filterpreset.h \
	core/divelist.h \
	core/divelogexportlogic.h \
	core/divesitehelpers.h \
	core/exif.h \
	core/file.h \
	core/fulltext.h \
	core/gaspressures.h \
	core/gettext.h \
	core/gettextfromc.h \
	core/membuffer.h \
	core/metrics.h \
	core/qt-gui.h \
	core/sample.h \
	core/selection.h \
	core/sha1.h \
	core/strndup.h \
	core/string-format.h \
	core/subsurfacestartup.h \
	core/subsurfacesysinfo.h \
	core/taxonomy.h \
	core/uemis.h \
	core/webservice.h \
	core/windowtitleupdate.h \
	core/worldmap-options.h \
	core/worldmap-save.h \
	core/downloadfromdcthread.h \
	core/btdiscovery.h \
	core/connectionlistmodel.h \
	core/qt-ble.h \
	core/save-profiledata.h \
	core/uploadDiveShare.h \
	core/uploadDiveLogsDE.h \
	core/xmlparams.h \
	core/settings/qPref.h \
	core/settings/qPrefCloudStorage.h \
	core/settings/qPrefDisplay.h \
	core/settings/qPrefDiveComputer.h \
	core/settings/qPrefDivePlanner.h \
	core/settings/qPrefGeneral.h \
	core/settings/qPrefGeocoding.h \
	core/settings/qPrefLanguage.h \
	core/settings/qPrefLocationService.h \
	core/settings/qPrefPartialPressureGas.h \
	core/settings/qPrefPrivate.h \
	core/settings/qPrefProxy.h \
	core/settings/qPrefTechnicalDetails.h \
	core/settings/qPrefUnit.h \
	core/settings/qPrefEquipment.h \
	core/settings/qPrefLog.h \
	core/settings/qPrefMedia.h \
	core/settings/qPrefUpdateManager.h \
	core/subsurface-qt/divelistnotifier.h \
	backend-shared/exportfuncs.h \
	backend-shared/plannershared.h \
	backend-shared/roundrectitem.h \
	stats/barseries.h \
	stats/boxseries.h \
	stats/chartitem.h \
	stats/chartlistmodel.h \
	stats/histogrammarker.h \
	stats/informationbox.h \
	stats/legend.h \
	stats/pieseries.h \
	stats/quartilemarker.h \
	stats/regressionitem.h \
	stats/scatterseries.h \
	stats/statsaxis.h \
	stats/statscolors.h \
	stats/statsgrid.h \
	stats/statshelper.h \
	stats/statsselection.h \
	stats/statsseries.h \
	stats/statsstate.h \
	stats/statstranslations.h \
	stats/statsvariables.h \
	stats/statsview.h \
	stats/zvalues.h \
	mobile-widgets/qmlinterface.h \
	mobile-widgets/qmlmanager.h \
	mobile-widgets/statsmanager.h \
	mobile-widgets/themeinterface.h \
	map-widget/qmlmapwidgethelper.h \
	qt-models/divesummarymodel.h \
	qt-models/diveplotdatamodel.h \
	qt-models/gpslistmodel.h \
	qt-models/divelocationmodel.h \
	qt-models/completionmodels.h \
	qt-models/weightsysteminfomodel.h \
	qt-models/maplocationmodel.h \
	qt-models/diveimportedmodel.h \
	qt-models/messagehandlermodel.h \
	qt-models/diveplannermodel.h \
	qt-models/divetripmodel.h \
	qt-models/mobilelistmodel.h \
	qt-models/cylindermodel.h \
	qt-models/cleanertablemodel.h \
	qt-models/tankinfomodel.h \
	qt-models/models.h \
	qt-models/weightsysteminfomodel.h \
	qt-models/filterconstraintmodel.h \
	qt-models/filterpresetmodel.h \
	profile-widget/qmlprofile.h \
	profile-widget/diveprofileitem.h \
	profile-widget/profilewidget2.h \
	profile-widget/ruleritem.h \
	profile-widget/diveeventitem.h \
	profile-widget/divetooltipitem.h \
	profile-widget/tankitem.h \
	profile-widget/animationfunctions.h \
	profile-widget/divecartesianaxis.h \
	profile-widget/divehandler.h \
	profile-widget/divelineitem.h \
	profile-widget/divepixmapitem.h \
	profile-widget/diverectitem.h \
	profile-widget/divetextitem.h

RESOURCES += mobile-widgets/qml/mobile-resources.qrc \
		mobile-widgets/3rdparty/icons.qrc \
		map-widget/qml/map-widget.qrc \
		stats/statsicons.qrc

android {
	SOURCES += core/android.cpp \
		core/serial_usb_android.cpp

	# ironically, we appear to need to include the Kirigami shaders here
	# as they aren't found when we assume that they are part of the
	# libkirigami library
	RESOURCES += packaging/android/translations.qrc \
		android-mobile/font.qrc \
		mobile-widgets/3rdparty/kirigami/src/scenegraph/shaders/shaders.qrc
	QT += androidextras
	ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android-mobile
	ANDROID_VERSION_CODE = $$BUILD_NR
	ANDROID_VERSION_NAME = $$BUILD_VERSION_NAME

	DISTFILES += \
		android-build/AndroidManifest.xml \
		android-build/build.gradle \
		android-build/res/values/libs.xml

	# at link time our CWD is parallel to the install-root
	LIBS += ../install-root-$${QT_ARCH}/lib/libdivecomputer.a \
		../install-root-$${QT_ARCH}/lib/qml/org/kde/kirigami.2/libkirigamiplugin.a \
		../install-root-$${QT_ARCH}/lib/libgit2.a \
		../install-root-$${QT_ARCH}/lib/libzip.a \
		../install-root-$${QT_ARCH}/lib/libxslt.a \
		../install-root-$${QT_ARCH}/lib/libxml2.a \
		../install-root-$${QT_ARCH}/lib/libsqlite3.a \
		../install-root-$${QT_ARCH}/lib/libssl_1_1.so \
		../install-root-$${QT_ARCH}/lib/libcrypto_1_1.so \
		../googlemaps-build/libplugins_geoservices_qtgeoservices_googlemaps_$${QT_ARCH}.so

	# ensure that the openssl libraries are bundled into the app
	# for some reason doing so with dollar dollar { QT_ARCH } (like what works
	# above for the link time case) doesn not work for the EXTRA_LIBS case.
	# so stupidly do it explicitly
	ANDROID_EXTRA_LIBS += \
		../install-root-arm64-v8a/lib/libcrypto_1_1.so \
		../install-root-arm64-v8a/lib/libssl_1_1.so \
		../install-root-armeabi-v7a/lib/libcrypto_1_1.so \
		../install-root-armeabi-v7a/lib/libssl_1_1.so

	INCLUDEPATH += ../install-root-$${QT_ARCH}/include/ \
		../install-root/lib/libzip/include \
		../install-root-$${QT_ARCH}/include/libxstl \
		../install-root-$${QT_ARCH}/include/libxml2 \
		../install-root-$${QT_ARCH}/include/libexstl \
		../install-root-$${QT_ARCH}/include/openssl \
		. \
		core \
		mobile-widgets/3rdparty/kirigami/src
}

ios {
	SOURCES += core/ios.cpp
	RESOURCES += packaging/ios/translations.qrc
	QMAKE_IOS_DEPLOYMENT_TARGET = 10.0
	QMAKE_TARGET_BUNDLE_PREFIX = org.subsurface-divelog
	QMAKE_BUNDLE = subsurface-mobile
	QMAKE_INFO_PLIST = packaging/ios/Info.plist
	QMAKE_ASSET_CATALOGS += packaging/ios/storeIcon.xcassets
	app_launch_images.files = packaging/ios/SubsurfaceMobileLaunch.xib $$files(packaging/ios/SubsurfaceMobileLaunchImage*.png)
	images.files = icons/subsurface-mobile-icon.png
	QMAKE_BUNDLE_DATA += app_launch_images images

	LIBS += ../install-root/ios/lib/libdivecomputer.a \
		../install-root/ios/lib/libgit2.a \
		../install-root/ios/lib/libzip.a \
		../install-root/ios/lib/libxslt.a \
		../install-root/ios/lib/qml/org/kde/kirigami.2/libkirigamiplugin.a \
		../googlemaps-build/libqtgeoservices_googlemaps.a \
		-liconv \
		-lsqlite3 \
		-lxml2

	INCLUDEPATH += ../install-root/ios/include/ \
		../install-root/lib/libzip/include \
		../install-root/ios/include/libxstl \
		../install-root/ios/include/libexstl \
		../install-root/ios/include/openssl \
		. \
		./core \
		./mobile-widgets/3rdparty/kirigami/src/libkirigami \
		/usr/include/libxml2

}
