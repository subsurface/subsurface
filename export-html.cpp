// SPDX-License-Identifier: GPL-2.0
/* Dirk Hohndel, 2015 */

#include <QString>
#include <QCommandLineParser>
#include <QApplication>

#include "core/qt-gui.h"
#include "core/qthelper.h"
#include "core/file.h"
#include "core/device.h"
#include "core/divelog.h"
#include "core/divesite.h"
#include "core/trip.h"
#include "core/save-html.h"
#include <stdio.h>
#include "git2.h"
#include "core/subsurfacestartup.h"
#include "core/divelogexportlogic.h"
#include "core/statistics.h"

int main(int argc, char **argv)
{
	QApplication *application = new QApplication(argc, argv);
	git_libgit2_init();
	prefs = default_prefs;
	init_qt_late();

	QCommandLineParser parser;
	QCommandLineOption sourceDirectoryOption(QStringList() << "s" << "source",
						 "Read git repository from <directory>",
						 "directory");
	parser.addOption(sourceDirectoryOption);
	QCommandLineOption outputDirectoryOption(QStringList() << "u" << "output",
						 "Write HTML files into <directory>",
						 "directory");
	parser.addOption(outputDirectoryOption);

	parser.process(*application);

	QString source = parser.value(sourceDirectoryOption);
	QString output = parser.value(outputDirectoryOption);

	if (source.isEmpty() || output.isEmpty()) {
		report_info("need --source and --output");
		exit(1);
	}
	int ret = parse_file(qPrintable(source), &divelog);
	if (ret) {
		report_info("parse_file returned %d", ret);
		exit(1);
	}

	// this should have set up the informational preferences - let's grab
	// the units from there
	prefs.unit_system = git_prefs.unit_system;
	prefs.units = git_prefs.units;

	// now set up the export settings to create the HTML export
	struct htmlExportSetting hes;
	hes.themeFile = "sand.css";
	hes.exportPhotos = false;
	hes.selectedOnly = false;
	hes.listOnly = false;
	hes.yearlyStatistics = true;
	hes.subsurfaceNumbers = true;
	exportHtmlInitLogic(output, hes);
	exit(0);
}
