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

void init_ui()
{
	init_qt_late();

	PluginManager::instance().loadPlugins();

	MainWindow *window = new MainWindow();
	window->setTitle();
}

void run_ui()
{
	MainWindow::instance()->show();
	qApp->exec();
}

void exit_ui()
{
	delete MainWindow::instance();
	delete qApp;
	free((void *)existing_filename);
}

double get_screen_dpi()
{
	QDesktopWidget *mydesk = qApp->desktop();
	return mydesk->physicalDpiX();
}


