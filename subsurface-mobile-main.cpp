/* main.c */
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "dive.h"
#include "qt-gui.h"
#include "subsurfacestartup.h"
#include "subsurface-core/color.h"
#include "qthelper.h"
#include "helpers.h"

#include <QStringList>
#include <QApplication>
#include <QLoggingCategory>
#include <git2.h>

QTranslator *qtTranslator, *ssrfTranslator;

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
	if (uiLanguage(0).contains("-US"))
		default_prefs.units = IMPERIAL_units;
	prefs = default_prefs;
	fill_profile_color();
	parse_xml_init();
	taglist_init_global();
	init_ui();
	// load the preferences and make sure to load the password
	// the mobile UI makes no sense without that
	prefs.save_password_local = true;
	loadPreferences();

	// some hard coded settings
	prefs.save_password_local = true; // in case somehow the loaded prefs reset that
	prefs.animation_speed = 0; // we render the profile to pixmap, no animations

	// always show the divecomputer reported ceiling in red
	prefs.dcceiling = 1;
	prefs.redceiling = 1;

	init_proxy();
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

	if (!quit)
		run_ui();
	exit_ui();
	taglist_free(g_tag_list);
	parse_xml_exit();
	subsurface_console_exit();
	free_prefs();
	return 0;
}
