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

	setup_system_prefs();
	prefs = default_prefs;

	init_ui(&argc, &argv);
	parse_xml_init();
	taglist_init_global();

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
			files.push_back( QString(prefs.default_filename) );
	}
	parse_xml_exit();
	mainWindow()->loadFiles(files);
	mainWindow()->importFiles(importedFiles);
	run_ui();
	exit_ui();
	return 0;
}
