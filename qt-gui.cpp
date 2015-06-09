/* qt-gui.cpp */
/* Qt UI implementation */
#include "dive.h"
#include "display.h"
#include "qt-ui/mainwindow.h"
#include "helpers.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QNetworkProxy>
#include <QLibraryInfo>


#include "qt-gui.h"

#ifdef SUBSURFACE_MOBILE
	#include <QQuickWindow>
	#include <QQmlApplicationEngine>
	#include <QQmlContext>
	#include "qt-mobile/qmlmanager.h"
	#include "qt-models/divelistmodel.h"
#endif

static MainWindow *window = NULL;

void init_ui()
{
	init_qt_late();

	window = new MainWindow();
	if (existing_filename && existing_filename[0] != '\0')
		window->setTitle(MWTF_FILENAME);
	else
		window->setTitle(MWTF_DEFAULT);
}

void run_ui()
{
#ifdef SUBSURFACE_MOBILE
	window->hide();
	qmlRegisterType<QMLManager>("org.subsurfacedivelog.mobile", 1, 0, "QMLManager");
	QQmlApplicationEngine engine;
	DiveListModel diveListModel;
	QQmlContext *ctxt = engine.rootContext();
	ctxt->setContextProperty("diveModel", &diveListModel);
	engine.load(QUrl(QStringLiteral("qrc:///qml/main.qml")));
	QObject *mainWindow = engine.rootObjects().value(0);
	QQuickWindow *qml_window = qobject_cast<QQuickWindow *>(mainWindow);
	qml_window->show();
#else
	window->show();
#endif
	qApp->exec();
}

void exit_ui()
{
	delete window;
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


