// SPDX-License-Identifier: GPL-2.0
/* main.c */
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "core/color.h"
#include "core/downloadfromdcthread.h"
#include "core/qt-gui.h"
#include "core/qthelper.h"
#include "core/subsurfacestartup.h"
#include "core/settings/qPref.h"
#include "core/settings/qPrefDisplay.h"
#include "core/tag.h"

#include <QApplication>
#include <QLocale>
#include <QLoggingCategory>
#include <QStringList>
#include <git2.h>

// Implementation of STP logging
#include "core/ssrf.h"
#ifdef ENABLE_STARTUP_TIMING
#include <QElapsedTimer>
#include <QMutex>
#include <QMutexLocker>
void log_stp(const char *ident, QString *buf)
{
	static bool firstCall = true;
	static QElapsedTimer stpDuration;
	static QString stpText;
	static QMutex logMutex;

	QMutexLocker l(&logMutex);

	if (firstCall) {
		firstCall = false;
		stpDuration.start();
	}
	if (ident)
		stpText += QString("STP ")
				   .append(QString::number(stpDuration.elapsed()))
				   .append(" ms, ")
				   .append(ident)
				   .append("\n");
	if (buf) {
		*buf += "---------- startup timer ----------\n";
		*buf += stpText;
	}
}
#endif // ENABLE_STARTUP_TIMING

int main(int argc, char **argv)
{
	LOG_STP("main starting");

	int i;
	QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth* = true"));

	// Start application
	std::unique_ptr<QApplication> app(new QApplication(argc, argv));
	LOG_STP("main Qt started");

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
	LOG_STP("main git loaded");
	setup_system_prefs();
	if (QLocale().measurementSystem() == QLocale::MetricSystem)
		default_prefs.units = SI_units;
	else
		default_prefs.units = IMPERIAL_units;
	copy_prefs(&default_prefs, &prefs);
	fill_computer_list();

	parse_xml_init();
	LOG_STP("main xml parsed");
	taglist_init_global();
	LOG_STP("main taglist done");
	init_ui();
	LOG_STP("main init_ui done");
	if (prefs.default_file_behavior == LOCAL_DEFAULT_FILE)
		set_filename(prefs.default_filename);
	else
		set_filename(NULL);

	// some hard coded settings
	qPrefDisplay::set_animation_speed(0); // we render the profile to pixmap, no animations

	// always show the divecomputer reported ceiling in red
	prefs.redceiling = 1;

	init_proxy();

	LOG_STP("main call run_ui (continue in qmlmanager)");
	if (!quit)
		run_ui();
	exit_ui();
	taglist_free(g_tag_list);
	parse_xml_exit();
	subsurface_console_exit();

	// Sync struct preferences to disk
	qPref::sync();

	free_prefs();
	return 0;
}

void set_non_bt_addresses()
{
#if SERIAL_FTDI
	connectionListModel.addAddress("FTDI");
#endif
#if defined(Q_OS_ANDROID)
#elif defined(Q_OS_LINUX) // since this is in the else, it does NOT include Android
	connectionListModel.addAddress("/dev/ttyS0");
	connectionListModel.addAddress("/dev/ttyS1");
	connectionListModel.addAddress("/dev/ttyS2");
	connectionListModel.addAddress("/dev/ttyS3");
	// this makes debugging so much easier - use the simulator
	connectionListModel.addAddress("/tmp/ttyS1");
#endif
}

bool haveFilesOnCommandLine()
{
	return false;
}
