// SPDX-License-Identifier: GPL-2.0
/* qt-gui.cpp */
/* Qt UI implementation */
#include "core/dive.h"
#include "core/display.h"
#include "desktop-widgets/mainwindow.h"
#include "core/helpers.h"
#include "core/pluginmanager.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QNetworkProxy>
#include <QLibraryInfo>

#include "core/qt-gui.h"

#ifdef SUBSURFACE_MOBILE
#include <QQuickWindow>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSortFilterProxyModel>
#include "mobile-widgets/qmlmanager.h"
#include "qt-models/divelistmodel.h"
#include "mobile-widgets/qmlprofile.h"
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
}

double get_screen_dpi()
{
	QDesktopWidget *mydesk = qApp->desktop();
	return mydesk->physicalDpiX();
}


