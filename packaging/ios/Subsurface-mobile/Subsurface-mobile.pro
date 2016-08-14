TEMPLATE = app

QT += qml quick widgets positioning concurrent svg

DEFINES += SUBSURFACE_MOBILE

CONFIG += c++11

SOURCES += ../../../subsurface-mobile-main.cpp \
    ../../../subsurface-mobile-helper.cpp \
    ../../../core/cloudstorage.cpp \
    ../../../core/configuredivecomputerthreads.cpp \
    ../../../core/devicedetails.cpp \
    ../../../core/gpslocation.cpp \
    ../../../core/imagedownloader.cpp \
    ../../../core/qthelper.cpp \
    ../../../core/checkcloudconnection.cpp \
    ../../../core/color.cpp \
    ../../../core/configuredivecomputer.cpp \
    ../../../core/divecomputer.cpp \
    ../../../core/divelogexportlogic.cpp \
    ../../../core/divesite.cpp \
    ../../../core/divesitehelpers.cpp \
    ../../../core/exif.cpp \
    ../../../core/gettextfromc.cpp \
    ../../../core/isocialnetworkintegration.cpp \
    ../../../core/metrics.cpp \
    ../../../core/pluginmanager.cpp \
    ../../../core/qt-init.cpp \
    ../../../core/subsurfacesysinfo.cpp \
    ../../../core/windowtitleupdate.cpp \
    ../../../core/file.c \
    ../../../core/subsurfacestartup.c \
    ../../../core/macos.c \
    ../../../core/profile.c \
    ../../../core/device.c \
    ../../../core/dive.c \
    ../../../core/divelist.c \
    ../../../core/gas-model.c \
    ../../../core/gaspressures.c \
    ../../../core/git-access.c \
    ../../../core/liquivision.c \
    ../../../core/load-git.c \
    ../../../core/parse-xml.c \
    ../../../core/save-html.c \
    ../../../core/statistics.c \
    ../../../core/worldmap-save.c \
    ../../../core/libdivecomputer.c \
    ../../../core/version.c \
    ../../../core/save-git.c \
    ../../../core/datatrak.c \
    ../../../core/ostctools.c \
    ../../../core/planner.c \
    ../../../core/save-xml.c \
    ../../../core/cochran.c \
    ../../../core/deco.c \
    ../../../core/divesite.c \
    ../../../core/equipment.c \
    ../../../core/membuffer.c \
    ../../../core/sha1.c \
    ../../../core/strtod.c \
    ../../../core/taxonomy.c \
    ../../../core/time.c \
    ../../../core/uemis.c \
    ../../../core/subsurface-qt/CylinderObjectHelper.cpp \
    ../../../core/subsurface-qt/DiveObjectHelper.cpp \
    ../../../core/subsurface-qt/SettingsObjectWrapper.cpp \
    ../../../mobile-widgets/qmlmanager.cpp \
    ../../../mobile-widgets/qmlprofile.cpp \
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

RESOURCES += qml.qrc ../../../subsurface.qrc ../../../mobile-widgets/qml/mobile-resources.qrc translations.qrc

LIBS += ../install-root/lib/libcrypto.a \
        ../install-root/lib/libdivecomputer.a \
        ../install-root/lib/libgit2.a \
        ../install-root/lib/libsqlite3.a \
        ../install-root/lib/libzip.a \
        ../install-root/lib/libxslt.a \
        ../install-root/lib/libxml2.a \
        ../install-root/lib/libssh2.a \
        ../install-root/lib/libssl.a \
        ../install-root/lib/libkirigamiplugin.a \
        -liconv

INCLUDEPATH += ../install-root/include/ \
               ../install-root/lib/libzip/include \
               ../install-root/include/libxml2 \
               ../install-root/include/libxstl \
               ../install-root/include/libexstl \
               ../install-root/include/openssl \
               ../../.. \
               ../../../core

HEADERS += \
    ../../../core/libdivecomputer.h \
    ../../../core/cloudstorage.h \
    ../../../core/configuredivecomputerthreads.h \
    ../../../core/device.h \
    ../../../core/devicedetails.h \
    ../../../core/dive.h \
    ../../../core/git-access.h \
    ../../../core/gpslocation.h \
    ../../../core/helpers.h \
    ../../../core/imagedownloader.h \
    ../../../core/pref.h \
    ../../../core/profile.h \
    ../../../core/qthelper.h \
    ../../../core/save-html.h \
    ../../../core/statistics.h \
    ../../../core/units.h \
    ../../../core/qthelperfromc.h \
    ../../../core/version.h \
    ../../../core/planner.h \
    ../../../core/divesite.h \
    ../../../core/checkcloudconnection.h \
    ../../../core/cochran.h \
    ../../../core/color.h \
    ../../../core/configuredivecomputer.h \
    ../../../core/datatrak.h \
    ../../../core/deco.h \
    ../../../core/display.h \
    ../../../core/divecomputer.h \
    ../../../core/divelist.h \
    ../../../core/divelogexportlogic.h \
    ../../../core/divesitehelpers.h \
    ../../../core/exif.h \
    ../../../core/file.h \
    ../../../core/gaspressures.h \
    ../../../core/gettext.h \
    ../../../core/gettextfromc.h \
    ../../../core/isocialnetworkintegration.h \
    ../../../core/membuffer.h \
    ../../../core/metrics.h \
    ../../../core/pluginmanager.h \
    ../../../core/prefs-macros.h \
    ../../../core/qt-gui.h \
    ../../../core/sha1.h \
    ../../../core/strndup.h \
    ../../../core/subsurfacestartup.h \
    ../../../core/subsurfacesysinfo.h \
    ../../../core/taxonomy.h \
    ../../../core/uemis.h \
    ../../../core/webservice.h \
    ../../../core/windowtitleupdate.h \
    ../../../core/worldmap-options.h \
    ../../../core/worldmap-save.h \
    ../../../core/subsurface-qt/CylinderObjectHelper.h \
    ../../../core/subsurface-qt/DiveObjectHelper.h \
    ../../../core/subsurface-qt/SettingsObjectWrapper.h \
    ../../../mobile-widgets/qmlmanager.h \
    ../../../mobile-widgets/qmlprofile.h \
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
