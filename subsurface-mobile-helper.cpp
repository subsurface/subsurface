/* qt-gui.cpp */
/* Qt UI implementation */
#include "dive.h"
#include "display.h"
#include "helpers.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QNetworkProxy>
#include <QLibraryInfo>

#include "qt-gui.h"

#include <QQuickWindow>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSortFilterProxyModel>
#include "qt-mobile/qmlmanager.h"
#include "qt-models/divelistmodel.h"
#include "qt-mobile/qmlprofile.h"

QObject *qqWindowObject = NULL;

void init_ui()
{
	init_qt_late();
}

void run_ui()
{
	qmlRegisterType<QMLManager>("org.subsurfacedivelog.mobile", 1, 0, "QMLManager");
	qmlRegisterType<QMLProfile>("org.subsurfacedivelog.mobile", 1, 0, "QMLProfile");
	QQmlApplicationEngine engine;
#if __APPLE__
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
#endif
	engine.addImportPath("qrc://imports");
	DiveListModel diveListModel;
	QSortFilterProxyModel *sortModel = new QSortFilterProxyModel(0);
	sortModel->setSourceModel(&diveListModel);
	sortModel->setDynamicSortFilter(true);
	sortModel->setSortRole(DiveListModel::DiveDateRole);
	sortModel->sort(0, Qt::DescendingOrder);
	QQmlContext *ctxt = engine.rootContext();
	ctxt->setContextProperty("diveModel", sortModel);
	engine.load(QUrl(QStringLiteral("qrc:///qml/main.qml")));
	qqWindowObject = engine.rootObjects().value(0);
	if (!qqWindowObject) {
		fprintf(stderr, "can't create window object\n");
		exit(1);
	}
	QQuickWindow *qml_window = qobject_cast<QQuickWindow *>(qqWindowObject);
	qml_window->setIcon(QIcon(":/subsurface-mobile-icon"));
	qqWindowObject->setProperty("messageText", QVariant("Subsurface mobile startup"));
#if !defined(Q_OS_ANDROID)
	qml_window->setHeight(1200);
	qml_window->setWidth(800);
#endif
	qml_window->show();
	qApp->exec();
}

void exit_ui()
{
	delete qApp;
	free((void *)existing_filename);
	free((void *)default_dive_computer_vendor);
	free((void *)default_dive_computer_product);
	free((void *)default_dive_computer_device);
}

double get_screen_dpi()
{
	QDesktopWidget *mydesk = qApp->desktop();
	return mydesk->physicalDpiX();
}

bool haveFilesOnCommandLine()
{
	return false;
}
