TEMPLATE = app

QT += qml quick widgets positioning concurrent svg

DEFINES += SUBSURFACE_MOBILE

CONFIG += c++11

SOURCES += ../../../subsurface-mobile-main.cpp \
    ../../../subsurface-mobile-helper.cpp \
    ../../../subsurface-core/cloudstorage.cpp \
    ../../../subsurface-core/configuredivecomputerthreads.cpp \
    ../../../subsurface-core/devicedetails.cpp \
    ../../../subsurface-core/gpslocation.cpp \
    ../../../subsurface-core/imagedownloader.cpp \
    ../../../subsurface-core/qthelper.cpp \
    ../../../subsurface-core/checkcloudconnection.cpp \
    ../../../subsurface-core/color.cpp \
    ../../../subsurface-core/configuredivecomputer.cpp \
    ../../../subsurface-core/divecomputer.cpp \
    ../../../subsurface-core/divelogexportlogic.cpp \
    ../../../subsurface-core/divesite.cpp \
    ../../../subsurface-core/divesitehelpers.cpp \
    ../../../subsurface-core/exif.cpp \
    ../../../subsurface-core/gettextfromc.cpp \
    ../../../subsurface-core/isocialnetworkintegration.cpp \
    ../../../subsurface-core/metrics.cpp \
    ../../../subsurface-core/pluginmanager.cpp \
    ../../../subsurface-core/qt-init.cpp \
    ../../../subsurface-core/subsurfacesysinfo.cpp \
    ../../../subsurface-core/windowtitleupdate.cpp \
    ../../../subsurface-core/file.c \
    ../../../subsurface-core/subsurfacestartup.c \
    ../../../subsurface-core/macos.c \
    ../../../subsurface-core/profile.c \
    ../../../subsurface-core/device.c \
    ../../../subsurface-core/dive.c \
    ../../../subsurface-core/divelist.c \
    ../../../subsurface-core/gas-model.c \
    ../../../subsurface-core/gaspressures.c \
    ../../../subsurface-core/git-access.c \
    ../../../subsurface-core/liquivision.c \
    ../../../subsurface-core/load-git.c \
    ../../../subsurface-core/parse-xml.c \
    ../../../subsurface-core/save-html.c \
    ../../../subsurface-core/statistics.c \
    ../../../subsurface-core/worldmap-save.c \
    ../../../subsurface-core/libdivecomputer.c \
    ../../../subsurface-core/version.c \
    ../../../subsurface-core/save-git.c \
    ../../../subsurface-core/datatrak.c \
    ../../../subsurface-core/ostctools.c \
    ../../../subsurface-core/planner.c \
    ../../../subsurface-core/save-xml.c \
    ../../../subsurface-core/cochran.c \
    ../../../subsurface-core/deco.c \
    ../../../subsurface-core/divesite.c \
    ../../../subsurface-core/equipment.c \
    ../../../subsurface-core/membuffer.c \
    ../../../subsurface-core/sha1.c \
    ../../../subsurface-core/strtod.c \
    ../../../subsurface-core/taxonomy.c \
    ../../../subsurface-core/time.c \
    ../../../subsurface-core/uemis.c \
    ../../../subsurface-core/subsurface-qt/DiveObjectHelper.cpp \
    ../../../subsurface-core/subsurface-qt/SettingsObjectWrapper.cpp \
    ../../../qt-mobile/qmlmanager.cpp \
    ../../../qt-mobile/qmlprofile.cpp \
    ../../../qt-models/cylindermodel.cpp \
    ../../../qt-models/divelistmodel.cpp \
    ../../../qt-models/diveplotdatamodel.cpp \
    ../../../qt-models/gpslistmodel.cpp \
    ../../../qt-models/yearlystatisticsmodel.cpp \
    ../../../qt-models/diveplannermodel.cpp \
    ../../../qt-models/cleanertablemodel.cpp \
    ../../../qt-models/completionmodels.cpp \
    ../../../qt-models/divecomputerextradatamodel.cpp \
    ../../../qt-models/divecomputermodel.cpp \
    ../../../qt-models/divelocationmodel.cpp \
    ../../../qt-models/divepicturemodel.cpp \
    ../../../qt-models/divesitepicturesmodel.cpp \
    ../../../qt-models/divetripmodel.cpp \
    ../../../qt-models/filtermodels.cpp \
    ../../../qt-models/models.cpp \
    ../../../qt-models/tankinfomodel.cpp \
    ../../../qt-models/treemodel.cpp \
    ../../../qt-models/weightmodel.cpp \
    ../../../qt-models/weigthsysteminfomodel.cpp \
    ../../../qt-models/ssrfsortfilterproxymodel.cpp \
    ../../../profile-widget/divecartesianaxis.cpp \
    ../../../profile-widget/diveeventitem.cpp \
    ../../../profile-widget/diveprofileitem.cpp \
    ../../../profile-widget/profilewidget2.cpp \
    ../../../profile-widget/ruleritem.cpp \
    ../../../profile-widget/animationfunctions.cpp \
    ../../../profile-widget/divepixmapitem.cpp \
    ../../../profile-widget/divetooltipitem.cpp \
    ../../../profile-widget/tankitem.cpp \
    ../../../profile-widget/divelineitem.cpp \
    ../../../profile-widget/diverectitem.cpp \
    ../../../profile-widget/divetextitem.cpp

RESOURCES += qml.qrc ../../../subsurface.qrc ../../../qt-mobile/qml/mobile-resources.qrc

LIBS += ../install-root/lib/libcrypto.a \
        ../install-root/lib/libdivecomputer.a \
        ../install-root/lib/libgit2.a \
        ../install-root/lib/libsqlite3.a \
        ../install-root/lib/libzip.a \
        ../install-root/lib/libxslt.a \
        ../install-root/lib/libxml2.a \
        ../install-root/lib/libssh2.a \
        ../install-root/lib/libssl.a \
        -liconv

INCLUDEPATH += ../install-root/include/ \
               ../install-root/lib/libzip/include \
               ../install-root/include/libxml2 \
               ../install-root/include/libxstl \
               ../install-root/include/libexstl \
               ../install-root/include/openssl \
               ../../.. \
               ../../../subsurface-core

HEADERS += \
    ../../../subsurface-core/libdivecomputer.h \
    ../../../subsurface-core/cloudstorage.h \
    ../../../subsurface-core/configuredivecomputerthreads.h \
    ../../../subsurface-core/device.h \
    ../../../subsurface-core/devicedetails.h \
    ../../../subsurface-core/dive.h \
    ../../../subsurface-core/git-access.h \
    ../../../subsurface-core/gpslocation.h \
    ../../../subsurface-core/helpers.h \
    ../../../subsurface-core/imagedownloader.h \
    ../../../subsurface-core/pref.h \
    ../../../subsurface-core/profile.h \
    ../../../subsurface-core/qthelper.h \
    ../../../subsurface-core/save-html.h \
    ../../../subsurface-core/statistics.h \
    ../../../subsurface-core/units.h \
    ../../../subsurface-core/qthelperfromc.h \
    ../../../subsurface-core/version.h \
    ../../../subsurface-core/planner.h \
    ../../../subsurface-core/divesite.h \
    ../../../subsurface-core/checkcloudconnection.h \
    ../../../subsurface-core/cochran.h \
    ../../../subsurface-core/color.h \
    ../../../subsurface-core/configuredivecomputer.h \
    ../../../subsurface-core/datatrak.h \
    ../../../subsurface-core/deco.h \
    ../../../subsurface-core/display.h \
    ../../../subsurface-core/divecomputer.h \
    ../../../subsurface-core/divelist.h \
    ../../../subsurface-core/divelogexportlogic.h \
    ../../../subsurface-core/divesitehelpers.h \
    ../../../subsurface-core/exif.h \
    ../../../subsurface-core/file.h \
    ../../../subsurface-core/gaspressures.h \
    ../../../subsurface-core/gettext.h \
    ../../../subsurface-core/gettextfromc.h \
    ../../../subsurface-core/isocialnetworkintegration.h \
    ../../../subsurface-core/membuffer.h \
    ../../../subsurface-core/metrics.h \
    ../../../subsurface-core/pluginmanager.h \
    ../../../subsurface-core/prefs-macros.h \
    ../../../subsurface-core/qt-gui.h \
    ../../../subsurface-core/sha1.h \
    ../../../subsurface-core/strndup.h \
    ../../../subsurface-core/subsurfacestartup.h \
    ../../../subsurface-core/subsurfacesysinfo.h \
    ../../../subsurface-core/taxonomy.h \
    ../../../subsurface-core/uemis.h \
    ../../../subsurface-core/webservice.h \
    ../../../subsurface-core/windowtitleupdate.h \
    ../../../subsurface-core/worldmap-options.h \
    ../../../subsurface-core/worldmap-save.h \
    ../../../subsurface-core/subsurface-qt/DiveObjectHelper.h \
    ../../../subsurface-core/subsurface-qt/SettingsObjectWrapper.h \
    ../../../qt-mobile/qmlmanager.h \
    ../../../qt-mobile/qmlprofile.h \
    ../../../qt-models/divelistmodel.h \
    ../../../qt-models/diveplotdatamodel.h \
    ../../../qt-models/gpslistmodel.h \
    ../../../qt-models/divelocationmodel.h \
    ../../../qt-models/cylindermodel.h \
    ../../../qt-models/divecomputermodel.h \
    ../../../qt-models/diveplannermodel.h \
    ../../../qt-models/divetripmodel.h \
    ../../../qt-models/models.h \
    ../../../qt-models/weightmodel.h \
    ../../../qt-models/cleanertablemodel.h \
    ../../../qt-models/divepicturemodel.h \
    ../../../qt-models/ssrfsortfilterproxymodel.h \
    ../../../qt-models/divesitepicturesmodel.h \
    ../../../qt-models/completionmodels.h \
    ../../../qt-models/weigthsysteminfomodel.h \
    ../../../qt-models/divecomputerextradatamodel.h \
    ../../../qt-models/filtermodels.h \
    ../../../qt-models/tankinfomodel.h \
    ../../../qt-models/treemodel.h \
    ../../../qt-models/yearlystatisticsmodel.h \
    ../../../profile-widget/diveprofileitem.h \
    ../../../profile-widget/profilewidget2.h \
    ../../../profile-widget/ruleritem.h \
    ../../../profile-widget/diveeventitem.h \
    ../../../profile-widget/divetooltipitem.h \
    ../../../profile-widget/tankitem.h \
    ../../../profile-widget/animationfunctions.h \
    ../../../profile-widget/divecartesianaxis.h \
    ../../../profile-widget/divelineitem.h \
    ../../../profile-widget/divepixmapitem.h \
    ../../../profile-widget/diverectitem.h \
    ../../../profile-widget/divetextitem.h

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

ios {
    ios_icon.files = $$files(../../../icons/AppIcon*.png)
    app_launch_images.files = ../SubsurfaceMobileLaunch.xib $$files(../SubsurfaceMobileLaunchImage*.png)
    QMAKE_BUNDLE_DATA += app_launch_images ios_icon
    QMAKE_INFO_PLIST = ../Info.plist
}
