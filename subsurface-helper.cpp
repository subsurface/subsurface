// SPDX-License-Identifier: GPL-2.0
#include <QQmlEngine>
#include <QDebug>
#include <QQuickItem>

#include "core/qt-gui.h"
#include "core/settings/qPref.h"
#ifdef SUBSURFACE_MOBILE
#include "mobile-widgets/qmlmanager.h"
#include "mobile-widgets/qmlprefs.h"
#include "qt-models/divelistmodel.h"
#include "qt-models/gpslistmodel.h"
#include "profile-widget/qmlprofile.h"
#include "core/downloadfromdcthread.h"
#include "qt-models/diveimportedmodel.h"
#include "map-widget/qmlmapwidgethelper.h"
#include "qt-models/maplocationmodel.h"
#endif

void register_qml_types()
{
	int rc;
	rc = qmlRegisterType<qPref>("org.subsurfacedivelog.mobile", 1, 0, "SsrfPrefs");
	if (rc < 0)
		qDebug() << "ERROR: Cannot register Prefs (class qPref), QML will not work!!";
	rc = qmlRegisterType<qPrefDisplay>("org.subsurfacedivelog.mobile", 1, 0, "SsrfDisplayPrefs");
	if (rc < 0)
		qDebug() << "ERROR: Cannot register DisplayPrefs (class qPrefDisplay), QML will not work!!";

#ifdef SUBSURFACE_MOBILE
	rc = qmlRegisterType<QMLManager>("org.subsurfacedivelog.mobile", 1, 0, "QMLManager");
	if (rc < 0)
		qDebug() << "ERROR: Cannot register QMLManager, QML will not work!!";
	rc = qmlRegisterType<QMLPrefs>("org.subsurfacedivelog.mobile", 1, 0, "QMLPrefs");
	if (rc < 0)
		qDebug() << "ERROR: Cannot register QMLPrefs, QML will not work!!";
	rc = qmlRegisterType<QMLProfile>("org.subsurfacedivelog.mobile", 1, 0, "QMLProfile");
	if (rc < 0)
		qDebug() << "ERROR: Cannot register QMLProfile, QML will not work!!";
	rc = qmlRegisterType<DownloadThread>("org.subsurfacedivelog.mobile", 1, 0, "DCDownloadThread");
	if (rc < 0)
		qDebug() << "ERROR: Cannot register DCDownloadThread, QML will not work!!";
	rc = qmlRegisterType<DiveImportedModel>("org.subsurfacedivelog.mobile", 1, 0, "DCImportModel");
	if (rc < 0)
		qDebug() << "ERROR: Cannot register DCImportModel, QML will not work!!";

	rc = qmlRegisterType<MapWidgetHelper>("org.subsurfacedivelog.mobile", 1, 0, "MapWidgetHelper");
	if (rc < 0)
		qDebug() << "ERROR: Cannot register MapWidgetHelper, QML will not work!!";
	rc = qmlRegisterType<MapLocationModel>("org.subsurfacedivelog.mobile", 1, 0, "MapLocationModel");
	if (rc < 0)
		qDebug() << "ERROR: Cannot register MapLocationModel, QML will not work!!";
	rc = qmlRegisterType<MapLocation>("org.subsurfacedivelog.mobile", 1, 0, "MapLocation");
	if (rc < 0)
		qDebug() << "ERROR: Cannot register MapLocation, QML will not work!!";
#endif
}
