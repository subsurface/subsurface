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
	QApplication *application = init_qt(&argc, &argv);
	git_libgit2_init();
	setup_system_prefs();
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
	qDebug() << source << output;
	fprintf(stderr, "parse_file returned %d\n", parse_file(qPrintable(source)));

	struct htmlExportSetting hes;
	hes.themeFile = "sand.css";
	hes.exportPhotos = true;
	hes.selectedOnly = false;
	hes.listOnly = false;
	hes.yearlyStatistics = true;
	exportHtmlInitLogic(output, &hes);
	exit(0);
}
