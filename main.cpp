/* main.c */
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "qt-gui.h"
#include "subsurfacestartup.h"
#include "qt-ui/mainwindow.h"
#include "qt-ui/diveplanner.h"

#include <QStringList>

QTranslator *qtTranslator, *ssrfTranslator;

int main(int argc, char **argv)
{
	int i;
	bool no_filenames = true;

	init_qt(&argc, &argv);
	QStringList files;
	QStringList importedFiles;
	QStringList arguments = QCoreApplication::arguments();
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
	if (no_filenames) {
		QString defaultFile(prefs.default_filename);
		if (!defaultFile.isEmpty())
			files.push_back(QString(prefs.default_filename));
	}
	setup_system_prefs();
	prefs = default_prefs;
	fill_profile_color();
	parse_xml_init();
	taglist_init_global();
	init_ui();

	MainWindow *m = MainWindow::instance();
	m->setLoadedWithFiles(!files.isEmpty() || !importedFiles.isEmpty());
	m->loadFiles(files);
	m->importFiles(importedFiles);
	if (!quit)
		run_ui();
	exit_ui();
	parse_xml_exit();
	return 0;
}
