// SPDX-License-Identifier: GPL-2.0
/* main.c */
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "core/dive.h"
#include "core/color.h"
#include "core/downloadfromdcthread.h"
#include "core/parse.h"
#include "core/qt-gui.h"
#include "core/qthelper.h"
#include "core/subsurfacestartup.h"
#include "core/settings/qPref.h"
#include "core/settings/qPrefDisplay.h"
#include "core/tag.h"
#include "core/settings/qPrefCloudStorage.h"

#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QLocale>
#include <QLoggingCategory>
#include <QStringList>
#include <git2.h>

// Implementation of STP logging
#include "core/ssrf.h"

int main(int argc, char **argv)
{
	int i;
	QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth* = true"));

	// Start application
	std::unique_ptr<QApplication> app(new QApplication(argc, argv));

	// and get comand line arguments
	QStringList arguments = QCoreApplication::arguments();

	subsurface_console_init();

	for (i = 1; i < arguments.length(); i++) {
		QString a = arguments.at(i);
		if (!a.isEmpty() && a.at(0) == '-') {
			parse_argument(qPrintable(a));
			continue;
		}
	}
	git_libgit2_init();
	setup_system_prefs();
	if (QLocale().measurementSystem() == QLocale::MetricSystem)
		default_prefs.units = SI_units;
	else
		default_prefs.units = IMPERIAL_units;
	copy_prefs(&default_prefs, &prefs);
	fill_computer_list();
	reset_tank_info_table(&tank_info_table);

	parse_xml_init();
	taglist_init_global();

	// grab the system font size before we overwrite this when we load preferences
	double initial_font_size = QGuiApplication::font().pointSizeF();
	if (initial_font_size < 0.0) {
		// The OS provides a default font in pixels, not points; doing some crude math
		// to reverse engineer that information by measuring the height of a 10pt font in pixels
		QFont testFont;
		testFont.setPointSizeF(10.0);
		QFontMetrics fm(testFont);
		initial_font_size = QGuiApplication::font().pixelSize() * 10.0 / fm.height();
	}
	init_ui();
	if (prefs.default_file_behavior == LOCAL_DEFAULT_FILE)
		set_filename(prefs.default_filename);
	else
		set_filename(NULL);

	// some hard coded settings
	qPrefDisplay::set_animation_speed(0); // we render the profile to pixmap, no animations
	qPrefCloudStorage::set_save_password_local(true);

	// always show the divecomputer reported ceiling in red
	prefs.redceiling = 1;

	init_proxy();

	if (!quit)
		run_mobile_ui(initial_font_size);
	exit_ui();
	taglist_free(g_tag_list);
	parse_xml_exit();
	subsurface_console_exit();

	// Sync struct preferences to disk
	qPref::sync();

	free_prefs();
	clear_tank_info_table(&tank_info_table);
	return 0;
}

void set_non_bt_addresses()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
	connectionListModel.addAddress("/dev/ttyS0");
	connectionListModel.addAddress("/dev/ttyS1");
	connectionListModel.addAddress("/dev/ttyS2");
	connectionListModel.addAddress("/dev/ttyS3");
	// this makes debugging so much easier - use the simulator
	connectionListModel.addAddress("/tmp/ttyS1");
#endif
#if defined(SERIAL_FTDI)
	connectionListModel.addAddress("FTDI");
#endif
}
