/* Dirk Hohndel, 2015 */

#include <QString>
#include <QCommandLineParser>
#include <QDebug>

#include "qt-gui.h"
#include "dive.h"
#include "save-html.h"
#include "stdio.h"
#include "git2.h"
#include "subsurfacestartup.h"
#include "divelogexportlogic.h"

QTranslator *qtTranslator, *ssrfTranslator;

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
		qDebug() << "need --source and --output";
		exit(1);
	}

	int ret = parse_file(qPrintable(source));
	if (ret) {
		fprintf(stderr, "parse_file returned %d\n", ret);
		exit(1);
	}

	// this should have set up the informational preferences - let's grab
	// the units from there

	prefs.unit_system = informational_prefs.unit_system;
	prefs.units = informational_prefs.units;

	// now set up the export settings to create the HTML export
	struct htmlExportSetting hes;
	hes.themeFile = "sand.css";
	hes.exportPhotos = true;
	hes.selectedOnly = false;
	hes.listOnly = false;
	hes.yearlyStatistics = true;
	hes.subsurfaceNumbers = true;
	exportHtmlInitLogic(output, hes);
	exit(0);
}
