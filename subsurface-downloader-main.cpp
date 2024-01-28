// SPDX-License-Identifier: GPL-2.0
/* main.c */
#include "core/downloadfromdcthread.h" // for fill_computer_list
#include "core/errorhelper.h"
#include "core/parse.h"
#include "core/qthelper.h"
#include "core/subsurfacestartup.h"
#include "core/settings/qPref.h"
#include "core/tag.h"
#include "core/dive.h"
#include "core/divelog.h"
#include "core/subsurface-string.h"
#include "core/file.h"
#include "core/trip.h"
#include "core/libdivecomputer.h"
#include "commands/command.h"

#include <QApplication>
#include <QLoggingCategory>
#include <QStringList>
#include <git2.h>

static bool filesOnCommandLine = false;
static void messageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg);
extern void cliDownloader(const char *vendor, const char *product, const char *device);

int main(int argc, char **argv)
{
	Command::init();
	qInstallMessageHandler(messageHandler);
	// we always run this in verbose mode as there is no UI
	verbose = 1;

	// now let's say Hi
	print_version();

	// supporting BT makes sense when used with an iPhone and an rfcomm BT device?
	QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth* = true"));

	int i;
	bool no_filenames = true;
	std::unique_ptr<QCoreApplication> app(new QCoreApplication(argc, argv));
	QStringList files;
	QStringList importedFiles;
	QStringList arguments = QCoreApplication::arguments();

	// set a default logfile name for libdivecomputer so we always get a logfile
	logfile_name = strdup("subsurface-downloader.log");

	const char *default_directory = system_default_directory();
	subsurface_mkdir(default_directory);

	if (subsurface_user_is_root() && !force_root) {
		printf("You are running Subsurface as root. This is not recommended.\n");
		printf("If you insist to do so, run with option --allow_run_as_root.\n");
		exit(0);
	}
	git_libgit2_init();
	setup_system_prefs();
	copy_prefs(&default_prefs, &prefs);

	// now handle the arguments
	fill_computer_list();
	for (i = 1; i < arguments.length(); i++) {
		QString a = arguments.at(i);
		if (a.isEmpty())
			continue;
		if (a.at(0) == '-') {
			parse_argument(qPrintable(a));
			continue;
		}
		if (imported) {
			importedFiles.push_back(a);
		} else {
			no_filenames = false;
			files.push_back(a);
		}
	}
	parse_xml_init();
	taglist_init_global();

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
	filesOnCommandLine = !files.isEmpty() || !importedFiles.isEmpty();
	if (!files.isEmpty()) {
		qDebug() << "loading dive data from" << files;
		if (parse_file(qPrintable(files.first()), &divelog) < 0) {
			printf("Failed to load dives from file '%s', aborting.\n", qPrintable(files.first()));
			exit(1);
		}
	}
	print_files();
	if (!quit) {
		if (!empty_string(prefs.dive_computer.vendor) && !empty_string(prefs.dive_computer.product) && !empty_string(prefs.dive_computer.device)) {
			// download from that dive computer
			printf("Downloading dives from %s %s (via %s)\n", prefs.dive_computer.vendor, prefs.dive_computer.product, prefs.dive_computer.device);
			cliDownloader(prefs.dive_computer.vendor, prefs.dive_computer.product, prefs.dive_computer.device);
		}
	}
	if (!files.isEmpty()) {
		qDebug() << "saving dive data to" << files;
		save_dives(qPrintable(files.first()));
	}
	else {
		printf("No log files given, not saving dive data.\n");
		printf("Give a log file name as argument, or configure a cloud URL.\n");
	}
	clear_divelog(&divelog);
	taglist_free(g_tag_list);
	parse_xml_exit();

	// Sync struct preferences to disk
	qPref::sync();

	free_prefs();
	return 0;
}

// install this message handler primarily so that the Windows build can log to files
void messageHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
	QByteArray localMsg = msg.toUtf8();
	switch (type) {
	case QtDebugMsg:
		fprintf(stdout, "%s\n", localMsg.constData());
		break;
	case QtInfoMsg:
		fprintf(stdout, "%s\n", localMsg.constData());
		break;
	case QtWarningMsg:
		fprintf(stderr, "%s\n", localMsg.constData());
		break;
	case QtCriticalMsg:
		fprintf(stderr, "%s\n", localMsg.constData());
		break;
	case QtFatalMsg:
		fprintf(stderr, "%s\n", localMsg.constData());
		abort();
	}
}
