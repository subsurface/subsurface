// SPDX-License-Identifier: GPL-2.0
/* main.c */
#include "core/downloadfromdcthread.h" // for fill_computer_list
#include "core/errorhelper.h"
#include "core/parse.h"
#include "core/qthelper.h"
#include "core/settings/qPref.h"
#include "core/tag.h"
#include "core/dive.h"
#include "core/divelog.h"
#include "core/subsurfacestartup.h"
#include "core/subsurface-string.h"
#include "core/file.h"
#include "core/trip.h"
#include "core/libdivecomputer.h"
#include "commands/command.h"

#include <QApplication>
#include <QLoggingCategory>
#include <QStringList>
#include <git2.h>
#include "core/subsurfacestartup.h"

static void messageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg);
extern void cliDownloader(const std::string &vendor, const std::string &product, const std::string &device);
extern int cliFirmwareUpdate(const std::string &vendor, const std::string &product, const std::string &device, const std::string &firmwareFile, bool forceUpdate);

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

	bool no_filenames = true;
	std::unique_ptr<QCoreApplication> app(new QCoreApplication(argc, argv));
	std::vector<std::string> files;
	std::vector<std::string> importedFiles;
	QStringList arguments = QCoreApplication::arguments();

	// set a default logfile name for libdivecomputer so we always get a logfile
	logfile_name = "subsurface-downloader.log";

	subsurface_mkdir(system_default_directory().c_str());

	if (subsurface_user_is_root() && !force_root) {
		printf("You are running Subsurface as root. This is not recommended.\n");
		printf("If you insist to do so, run with option --allow_run_as_root.\n");
		exit(0);
	}
	git_libgit2_init();
	setup_system_prefs();
	prefs = default_prefs;

	// now handle the arguments
	fill_computer_list();
	for (int i = 1; i < arguments.length(); i++) {
		std::string a = arguments[i].toStdString();
		if (a.empty())
			continue;
		if (a[0] == '-') {
			parse_argument(a.c_str());
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

	// Skip file loading when doing firmware update
	if (!firmware_do_update) {
		if (no_filenames) {
			if (prefs.default_file_behavior == LOCAL_DEFAULT_FILE) {
				if (!prefs.default_filename.empty())
					files.emplace_back(prefs.default_filename.c_str());
			} else if (prefs.default_file_behavior == CLOUD_DEFAULT_FILE) {
				auto cloudURL = getCloudURL();
				if (cloudURL)
					files.emplace_back(*cloudURL);
			}
		}
		if (!files.empty()) {
			report_info("loading dive data from %s", join(files, ", ").c_str());
			if (parse_file(files.front().c_str(), &divelog) < 0) {
				printf("Failed to load dives from file '%s', aborting.\n", files.front().c_str());
				exit(1);
			}
		}
		print_files();
	}

	if (!quit) {
		if (firmware_do_update) {
			// perform firmware update
			if (prefs.dive_computer.vendor.empty() || prefs.dive_computer.product.empty() || prefs.dive_computer.device.empty() || firmware_file.empty()) {
				fprintf(stderr, "Firmware update needs --dc-vendor, --dc-product, --device and --firmware-file\n");
				exit(1);
			} else {
				printf("Updating firmware on %s %s (via %s) with file %s\n", prefs.dive_computer.vendor.c_str(), prefs.dive_computer.product.c_str(), prefs.dive_computer.device.c_str(), firmware_file.c_str());
				int rc = cliFirmwareUpdate(prefs.dive_computer.vendor, prefs.dive_computer.product, prefs.dive_computer.device, firmware_file, firmware_force_update);
				// No need to call parse_xml_exit() as we haven't loaded any dive data
				exit(rc);
			}
		} else if (!prefs.dive_computer.vendor.empty() && !prefs.dive_computer.product.empty() && !prefs.dive_computer.device.empty()) {
			// download from that dive computer
			printf("Downloading dives from %s %s (via %s)\n", prefs.dive_computer.vendor.c_str(),
					prefs.dive_computer.product.c_str(), prefs.dive_computer.device.c_str());
			cliDownloader(prefs.dive_computer.vendor, prefs.dive_computer.product, prefs.dive_computer.device);
		}
	}
	if (!files.empty()) {
		report_info("saving dive data to %s", join(files, ", ").c_str());
		save_dives(files.front().c_str());
	} else {
		printf("No log files given, not saving dive data.\n");
		printf("Give a log file name as argument, or configure a cloud URL.\n");
	}
	parse_xml_exit();

	// Sync struct preferences to disk
	qPref::sync();

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
