/* main.c */
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "dive.h"
#include "qt-gui.h"
#include "subsurfacestartup.h"
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/maintab.h"
#include "profile-widget/profilewidget2.h"
#include "preferences/preferencesdialog.h"
#include "desktop-widgets/diveplanner.h"
#include "subsurface-core/color.h"
#include "qthelper.h"

#include <QStringList>
#include <QApplication>
#include <QLoggingCategory>
#include <git2.h>

QTranslator *qtTranslator, *ssrfTranslator;

static bool filesOnCommandLine = false;

int main(int argc, char **argv)
{
	int i;
	bool no_filenames = true;
	QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth* = true"));
	QApplication *application = new QApplication(argc, argv);
	(void)application;
	QStringList files;
	QStringList importedFiles;
	QStringList arguments = QCoreApplication::arguments();

	bool dedicated_console = arguments.length() > 1 &&
				 (arguments.at(1) == QString("--win32console"));
	subsurface_console_init(dedicated_console);

	const char *default_directory = system_default_directory();
	const char *default_filename = system_default_filename();
	subsurface_mkdir(default_directory);

	for (i = 1; i < arguments.length(); i++) {
		QString a = arguments.at(i);
		if (a.isEmpty())
			continue;
		if (a.at(0) == '-') {
			parse_argument(a.toLocal8Bit().data());
			continue;
		}
		if (imported) {
			importedFiles.push_back(a);
		} else {
			no_filenames = false;
			files.push_back(a);
		}
	}
#if !LIBGIT2_VER_MAJOR && LIBGIT2_VER_MINOR < 22
	git_threads_init();
#else
	git_libgit2_init();
#endif
	setup_system_prefs();
	copy_prefs(&default_prefs, &prefs);
	fill_profile_color();
	parse_xml_init();
	taglist_init_global();
	init_ui();
	if (no_filenames) {
		if (prefs.default_file_behavior == LOCAL_DEFAULT_FILE) {
			QString defaultFile(prefs.default_filename);
			if (!defaultFile.isEmpty())
				files.push_back(QString(prefs.default_filename));
		} else if (prefs.default_file_behavior == CLOUD_DEFAULT_FILE) {
			QString cloudURL;
			if (getCloudURL(cloudURL) == 0)
				files.push_back(cloudURL);
		}
	}
	MainWindow *m = MainWindow::instance();
	filesOnCommandLine = !files.isEmpty() || !importedFiles.isEmpty();
	m->loadFiles(files);
	m->importFiles(importedFiles);
	// in case something has gone wrong make sure we show the error message
	m->showError();

	// now let's set up some connections
	QObject::connect(m->graphics(), &ProfileWidget2::enableToolbar ,m, &MainWindow::setEnabledToolbar, Qt::AutoConnection);
	QObject::connect(m->graphics(), &ProfileWidget2::showError, m, &MainWindow::showError, Qt::AutoConnection);
	QObject::connect(m->graphics(), &ProfileWidget2::disableShortcuts, m, &MainWindow::disableShortcuts, Qt::AutoConnection);
	QObject::connect(m->graphics(), &ProfileWidget2::enableShortcuts, m, &MainWindow::enableShortcuts, Qt::AutoConnection);
	QObject::connect(m->graphics(), &ProfileWidget2::refreshDisplay, m, &MainWindow::refreshDisplay, Qt::AutoConnection);
	QObject::connect(m->graphics(), &ProfileWidget2::updateDiveInfo, m->information(), &MainTab::updateDiveInfo, Qt::AutoConnection);
	QObject::connect(m->graphics(), &ProfileWidget2::editCurrentDive, m, &MainWindow::editCurrentDive, Qt::AutoConnection);
	QObject::connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), m->graphics(), SLOT(settingsChanged()));
	if (verbose > 0)
		print_files();
	if (!quit)
		run_ui();
	exit_ui();
	taglist_free(g_tag_list);
	parse_xml_exit();
	free((void *)default_directory);
	free((void *)default_filename);
	subsurface_console_exit();
	free_prefs();
	return 0;
}

bool haveFilesOnCommandLine()
{
	return filesOnCommandLine;
}
