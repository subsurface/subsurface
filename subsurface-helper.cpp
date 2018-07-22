// SPDX-License-Identifier: GPL-2.0
#include <QQmlEngine>
#include <QDesktopWidget>
#include <QApplication>
#include <QDebug>
#include <QQuickItem>

#include "map-widget/qmlmapwidgethelper.h"
#include "qt-models/maplocationmodel.h"
#include "core/qt-gui.h"
#include "core/settings/qPref.h"
#include "core/ssrf.h"

#ifdef SUBSURFACE_MOBILE
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "mobile-widgets/qmlmanager.h"
#include "mobile-widgets/qmlprefs.h"
#include "qt-models/divelistmodel.h"
#include "qt-models/gpslistmodel.h"
#include "qt-models/messagehandlermodel.h"
#include "profile-widget/qmlprofile.h"
#include "core/downloadfromdcthread.h"
#include "qt-models/diveimportedmodel.h"
#include "mobile-widgets/qml/kirigami/src/kirigamiplugin.h"
#else
#include "desktop-widgets/mainwindow.h"
#include "core/pluginmanager.h"
#endif

#ifndef SUBSURFACE_TEST_DATA
QObject *qqWindowObject = NULL;

static void register_meta_types();
void init_ui()
{
	init_qt_late();
	register_qml_types();
	register_meta_types();
#ifndef SUBSURFACE_MOBILE
	PluginManager::instance().loadPlugins();

	MainWindow *window = new MainWindow();
	window->setTitle();
#endif // SUBSURFACE_MOBILE
}

void exit_ui()
{
#ifndef SUBSURFACE_MOBILE
	delete MainWindow::instance();
#endif // SUBSURFACE_MOBILE
	delete qApp;
	free((void *)existing_filename);
}

double get_screen_dpi()
{
	QDesktopWidget *mydesk = qApp->desktop();
	return mydesk->physicalDpiX();
}

void run_ui()
{

#ifdef SUBSURFACE_MOBILE
	QQmlApplicationEngine engine;
	LOG_STP("run_ui qml engine started");
	KirigamiPlugin::getInstance().registerTypes();
#if defined(__APPLE__) && !defined(Q_OS_IOS)
	// when running the QML UI on a Mac the deployment of the QML Components seems
	// to fail and the search path for the components is rather odd - simply the
	// same directory the executable was started from <bundle>/Contents/MacOS/
	// To work around this we need to manually copy the components at install time
	// to Contents/Frameworks/qml and make sure that we add the correct import path
	QStringList importPathList = engine.importPathList();
	Q_FOREACH(QString importPath, importPathList) {
		if (importPath.contains("MacOS"))
			engine.addImportPath(importPath.replace("MacOS", "Frameworks"));
	}
	qDebug() << "QML import path" << engine.importPathList();
#endif // __APPLE__ not Q_OS_IOS
	engine.addImportPath("qrc://imports");
	DiveListModel diveListModel;
	LOG_STP("run_ui diveListModel started");
	DiveListSortModel *sortModel = new DiveListSortModel(0);
	sortModel->setSourceModel(&diveListModel);
	sortModel->setDynamicSortFilter(true);
	sortModel->setSortRole(DiveListModel::DiveDateRole);
	sortModel->sort(0, Qt::DescendingOrder);
	LOG_STP("run_ui diveListModel sorted");
	GpsListModel gpsListModel;
	QSortFilterProxyModel *gpsSortModel = new QSortFilterProxyModel(0);
	gpsSortModel->setSourceModel(&gpsListModel);
	gpsSortModel->setDynamicSortFilter(true);
	gpsSortModel->setSortRole(GpsListModel::GpsWhenRole);
	gpsSortModel->sort(0, Qt::DescendingOrder);
	QQmlContext *ctxt = engine.rootContext();
	ctxt->setContextProperty("diveModel", sortModel);
	ctxt->setContextProperty("gpsModel", gpsSortModel);
	ctxt->setContextProperty("vendorList", vendorList);
	set_non_bt_addresses();
	LOG_STP("run_ui set_non_bt_adresses");

	ctxt->setContextProperty("connectionListModel", &connectionListModel);
	ctxt->setContextProperty("logModel", MessageHandlerModel::self());

	engine.load(QUrl(QStringLiteral("qrc:///qml/main.qml")));
	LOG_STP("run_ui qml loaded");
	qqWindowObject = engine.rootObjects().value(0);
	if (!qqWindowObject) {
		fprintf(stderr, "can't create window object\n");
		exit(1);
	}
	QQuickWindow *qml_window = qobject_cast<QQuickWindow *>(qqWindowObject);
	qml_window->setIcon(QIcon(":subsurface-mobile-icon"));
	qqWindowObject->setProperty("messageText", QVariant("Subsurface-mobile startup"));
	qDebug() << "qqwindow devicePixelRatio" << qml_window->devicePixelRatio() << qml_window->screen()->devicePixelRatio();
	QScreen *screen = qml_window->screen();
	QObject::connect(qml_window, &QQuickWindow::screenChanged, QMLManager::instance(), &QMLManager::screenChanged);
	QMLManager *manager = QMLManager::instance();
	LOG_STP("run_ui qmlmanager instance started");
	// now that the log file is initialized...
	show_computer_list();
	LOG_STP("run_ui show_computer_list");

	manager->setDevicePixelRatio(qml_window->devicePixelRatio(), qml_window->screen());
	manager->dlSortModel = sortModel;
	manager->screenChanged(screen);
	qDebug() << "qqwindow screen has ldpi/pdpi" << screen->logicalDotsPerInch() << screen->physicalDotsPerInch();
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
	qml_window->setHeight(1200);
	qml_window->setWidth(800);
#endif // not Q_OS_ANDROID and not Q_OS_IOS
	qml_window->show();
	LOG_STP("run_ui running exec");
#else
	MainWindow::instance()->show();
#endif // SUBSURFACE_MOBILE
	qApp->exec();
}

Q_DECLARE_METATYPE(duration_t)
static void register_meta_types()
{
	qRegisterMetaType<duration_t>();
}
#endif // not SUBSURFACE_TEST_DATA

void register_qml_types()
{
	int rc;
	rc = qmlRegisterType<qPref>("org.subsurfacedivelog.mobile", 1, 0, "SsrfPrefs");
	if (rc < 0)
		qDebug() << "ERROR: Cannot register SsrfPrefs (class qPref), QML will not work!!";
	rc = qmlRegisterType<qPrefAnimations>("org.subsurfacedivelog.mobile", 1, 0, "SsrfAnimationsPrefs");
	if (rc < 0)
		qDebug() << "ERROR: Cannot register SsrfAnimationsPrefs (class qPrefAnimations), QML will not work!!";
	rc = qmlRegisterType<qPrefCloudStorage>("org.subsurfacedivelog.mobile", 1, 0, "SsrfCloudStoragePrefs");
	if (rc < 0)
		qDebug() << "ERROR: Cannot register SsrfCloudStoragePrefs (class qPrefCloudStorage), QML will not work!!";
	rc = qmlRegisterType<qPrefDisplay>("org.subsurfacedivelog.mobile", 1, 0, "SsrfDisplayPrefs");
	if (rc < 0)
		qDebug() << "ERROR: Cannot register DisplayPrefs (class qPrefDisplay), QML will not work!!";
	rc = qmlRegisterType<qPrefDiveComputer>("org.subsurfacedivelog.mobile", 1, 0, "SsrfDiveComputerPrefs");
	if (rc < 0)
		qDebug() << "ERROR: Cannot register DiveComputerPrefs (class qPrefDiveComputer), QML will not work!!";

#ifndef SUBSURFACE_TEST_DATA
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
#endif // not SUBSURFACE_MOBILE

	rc = qmlRegisterType<MapWidgetHelper>("org.subsurfacedivelog.mobile", 1, 0, "MapWidgetHelper");
	if (rc < 0)
		qDebug() << "ERROR: Cannot register MapWidgetHelper, QML will not work!!";
	rc = qmlRegisterType<MapLocationModel>("org.subsurfacedivelog.mobile", 1, 0, "MapLocationModel");
	if (rc < 0)
		qDebug() << "ERROR: Cannot register MapLocationModel, QML will not work!!";
	rc = qmlRegisterType<MapLocation>("org.subsurfacedivelog.mobile", 1, 0, "MapLocation");
	if (rc < 0)
		qDebug() << "ERROR: Cannot register MapLocation, QML will not work!!";
#endif // not SUBSURFACE_TEST_DATA
}
