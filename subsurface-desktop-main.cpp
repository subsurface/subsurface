/* main.c */
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "core/dive.h"
#include "core/qt-gui.h"
#include "core/subsurfacestartup.h"
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/maintab.h"
#include "profile-widget/profilewidget2.h"
#include "desktop-widgets/preferences/preferencesdialog.h"
#include "desktop-widgets/diveplanner.h"
#include "core/color.h"
#include "core/qthelper.h"

#include <QStringList>
#include <QApplication>
#include <QLoggingCategory>
#include <git2.h>

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

	bool win32_log = arguments.length() > 1 &&
		(arguments.at(1) == QString("--win32log"));
	if (win32_log) {
		subsurface_console_init(true, true);
	} else {
		bool dedicated_console = arguments.length() > 1 &&
			(arguments.at(1) == QString("--win32console"));
		subsurface_console_init(dedicated_console, false);
	}

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
	if (subsurface_user_is_root() && !force_root) {
		printf("You are running Subsurface as root. This is not recommended.\n");
		printf("If you insist to do so, run with option --allow_run_as_root.\n");
		exit(0);
	}
#if !LIBGIT2_VER_MAJOR && LIBGIT2_VER_MINOR < 22
	git_threads_init();
#else
	git_libgit2_init();
#endif
	/*
	 * Initialize the random number generator - not really secure as
	 * this is based only on current time, but it should not matter
	 * that much in our context. Moreover this is better than
	 * the constant numbers we used to get before.
	 */
	qsrand(time(NULL));

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
