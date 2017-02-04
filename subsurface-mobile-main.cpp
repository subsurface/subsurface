/* main.c */
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "core/dive.h"
#include "core/qt-gui.h"
#include "core/subsurfacestartup.h"
#include "core/color.h"
#include "core/qthelper.h"
#include "core/helpers.h"

#include <QStringList>
#include <QApplication>
#include <QLoggingCategory>
#include <git2.h>

int main(int argc, char **argv)
{
	int i;
	QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth* = true"));
	QApplication *application = new QApplication(argc, argv);
	(void)application;
	QStringList arguments = QCoreApplication::arguments();

	bool dedicated_console = arguments.length() > 1 &&
				 (arguments.at(1) == QString("--win32console"));
	subsurface_console_init(dedicated_console, false);

	for (i = 1; i < arguments.length(); i++) {
		QString a = arguments.at(i);
		if (!a.isEmpty() && a.at(0) == '-') {
			parse_argument(a.toLocal8Bit().data());
			continue;
		}
	}
	git_libgit2_init();
	setup_system_prefs();
	if (uiLanguage(0).contains("-US"))
		default_prefs.units = IMPERIAL_units;
	prefs = default_prefs;
	fill_profile_color();
	parse_xml_init();
	taglist_init_global();
	init_ui();
	if (prefs.default_file_behavior == LOCAL_DEFAULT_FILE)
		set_filename(prefs.default_filename, true);
	else
		set_filename(NULL, true);

	// some hard coded settings
	prefs.animation_speed = 0; // we render the profile to pixmap, no animations

	// always show the divecomputer reported ceiling in red
	prefs.dcceiling = 1;
	prefs.redceiling = 1;

	init_proxy();

	if (!quit)
		run_ui();
	exit_ui();
	taglist_free(g_tag_list);
	parse_xml_exit();
	subsurface_console_exit();
	free_prefs();
	return 0;
}
