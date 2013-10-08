/* main.c */
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "qt-gui.h"
#include "version.h"
#include "subsurfacestartup.h"
#include "qt-ui/mainwindow.h"
#include "qt-ui/diveplanner.h"

#include <QStringList>

int main(int argc, char **argv)
{
	int i;
	bool no_filenames = true;
#if 0
	const char *path;

	/* set up l18n - the search directory needs to change
	 * so that it uses the correct system directory when
	 * subsurface isn't run from the local directory */
	path = subsurface_gettext_domainpath(argv[0]);
	setlocale(LC_ALL, "");
	bindtextdomain("subsurface", path);
	bind_textdomain_codeset("subsurface", "utf-8");
	textdomain("subsurface");
#endif
	setup_system_prefs();
	prefs = default_prefs;

	init_ui(&argc, &argv);
	parse_xml_init();

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
