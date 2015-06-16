/* main.c */
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "dive.h"
#include "qt-gui.h"
#include "subsurfacestartup.h"
#include "qt-ui/mainwindow.h"
#include "qt-ui/diveplanner.h"
#include "qt-ui/graphicsview-common.h"
#include "qthelper.h"

#include <QStringList>
#include <QApplication>
#include <git2.h>

QTranslator *qtTranslator, *ssrfTranslator;

int main(int argc, char **argv)
{
	int i;
	bool no_filenames = true;

	QApplication *application = new QApplication(argc, argv);
	QStringList files;
	QStringList importedFiles;
	QStringList arguments = QCoreApplication::arguments();

	bool dedicated_console = arguments.length() > 1 &&
				 (arguments.at(1) == QString("--win32console"));
	subsurface_console_init(dedicated_console);

	for (i = 1; i < arguments.length(); i++) {
		QString a = arguments.at(i);
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
	prefs = default_prefs;
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
	m->setLoadedWithFiles(!files.isEmpty() || !importedFiles.isEmpty());
	m->loadFiles(files);
	m->importFiles(importedFiles);
	if (!quit)
		run_ui();
	exit_ui();
	taglist_free(g_tag_list);
	parse_xml_exit();
	subsurface_console_exit();
	free_prefs();
	return 0;
}
