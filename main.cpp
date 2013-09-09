/* main.c */
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <libintl.h>

#include "qt-gui.h"
#include "version.h"
#include "subsurfacestartup.h"
#include "qt-ui/mainwindow.h"

#include <QStringList>

int main(int argc, char **argv)
{
	int i;
	bool no_filenames = TRUE;
	const char *path;

	/* set up l18n - the search directory needs to change
	 * so that it uses the correct system directory when
	 * subsurface isn't run from the local directory */
	path = subsurface_gettext_domainpath(argv[0]);
	setlocale(LC_ALL, "");
	bindtextdomain("subsurface", path);
	bind_textdomain_codeset("subsurface", "utf-8");
	textdomain("subsurface");

	setup_system_prefs();
	prefs = default_prefs;

	subsurface_command_line_init(&argc, &argv);
	init_ui(&argc, &argv);
	parse_xml_init();

	QStringList files;
	QStringList importedFiles;
	for (i = 1; i < argc; i++) {
		const char *a = argv[i];
		if (a[0] == '-') {
			parse_argument(a);
			continue;
		}
		if (imported)
			importedFiles.push_back( QString(a) );
		else
			files.push_back( QString(a) );
	}
	if (no_filenames) {
		files.push_back( QString(prefs.default_filename) );
	}

	parse_xml_exit();
	subsurface_command_line_exit(&argc, &argv);
	mainWindow()->loadFiles(files);
	mainWindow()->importFiles(importedFiles);
	run_ui();
	exit_ui();
	return 0;
}
