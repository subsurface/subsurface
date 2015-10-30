/* qt-gui.cpp */
/* Qt UI implementation */
#include "dive.h"
#include "display.h"
#include "desktop-widgets/mainwindow.h"
#include "helpers.h"
#include "pluginmanager.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QNetworkProxy>
#include <QLibraryInfo>


#include "qt-gui.h"

#ifdef SUBSURFACE_MOBILE
#include <QQuickWindow>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSortFilterProxyModel>
#include "qt-mobile/qmlmanager.h"
#include "qt-models/divelistmodel.h"
#include "qt-mobile/qmlprofile.h"
QObject *qqWindowObject = NULL;
#endif

static MainWindow *window = NULL;

void init_ui()
{
	init_qt_late();

	PluginManager::instance().loadPlugins();

	window = new MainWindow();
	if (existing_filename && existing_filename[0] != '\0')
		window->setTitle(MWTF_FILENAME);
	else
		window->setTitle(MWTF_DEFAULT);
}

void run_ui()
{
	window->show();
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


