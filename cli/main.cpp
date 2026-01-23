// SPDX-License-Identifier: GPL-2.0
// subsurface-cli - Command-line interface for Subsurface dive log access
//
// This tool exposes Subsurface's core functionality via a JSON-based interface.
// It is designed to be called by the Flask web application.

#include "commands.h"
#include "config.h"
#include "core/divelog.h"
#include "core/subsurfacestartup.h"
#include "core/qthelper.h"
#include "core/errorhelper.h"
#include "core/tag.h"
#include "core/settings/qPref.h"
#include <QGuiApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <git2.h>

static void printUsage()
{
	fprintf(stderr, "Usage: subsurface-cli [--config=<path>] <command> [options]\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Commands:\n");
	fprintf(stderr, "  list-dives [--start=N] [--count=N]    List dives with pagination\n");
	fprintf(stderr, "  get-dive --dive-ref=<ref>             Get detailed dive information\n");
	fprintf(stderr, "  get-trip --trip-ref=<ref>             Get trip information with dives\n");
	fprintf(stderr, "  get-profile --dive-ref=<ref>          Generate dive profile image\n");
	fprintf(stderr, "             [--dc-index=N]\n");
	fprintf(stderr, "             [--width=N] [--height=N]\n");
	fprintf(stderr, "  get-stats                              Get overall statistics\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  --config=<path>    Path to config file (default: ~/.config/subsurface-cli/config.json)\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Dive/Trip Reference Format:\n");
	fprintf(stderr, "  Dive ref: yyyy/mm/dd-DDD-hh=mm=ss (e.g., 2024/06/15-Sat-10=30=00)\n");
	fprintf(stderr, "  Trip ref: yyyy/mm/dd-Location (e.g., 2024/06/15-Bonaire)\n");
}

int main(int argc, char *argv[])
{
	// QGuiApplication is needed for QGraphicsScene (used in profile rendering)
	// We use offscreen platform to avoid needing a display
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QGuiApplication app(argc, argv);
	QCoreApplication::setApplicationName("subsurface-cli");
	QCoreApplication::setApplicationVersion("1.0");

	// Initialize subsurface infrastructure
	git_libgit2_init();
	setup_system_prefs();
	prefs = default_prefs;
	taglist_init_global();

	// Parse command line
	QStringList args = QCoreApplication::arguments();
	if (args.size() < 2) {
		printUsage();
		return 1;
	}

	// Find config path and command
	QString configPath = defaultConfigPath();
	QString command;
	QStringList commandArgs;

	for (int i = 1; i < args.size(); i++) {
		const QString &arg = args[i];
		if (arg.startsWith("--config=")) {
			configPath = arg.mid(9);
		} else if (arg.startsWith("--")) {
			// Command option - pass to command
			commandArgs.append(arg);
		} else if (command.isEmpty()) {
			command = arg;
		} else {
			commandArgs.append(arg);
		}
	}

	if (command.isEmpty()) {
		printUsage();
		return 1;
	}

	// Load config
	CliConfig config = loadConfig(configPath);
	if (!config.isValid()) {
		fprintf(stderr, "Invalid or missing configuration.\n");
		fprintf(stderr, "Config file: %s\n", qPrintable(configPath));
		fprintf(stderr, "Required fields: repo_path, userid\n");
		return 1;
	}

	// Parse command-specific options
	int start = 0;
	int count = 50;
	QString diveRef;
	QString tripRef;
	int dcIndex = 0;
	int width = 1024;
	int height = 768;

	for (const QString &arg : commandArgs) {
		if (arg.startsWith("--start=")) {
			start = arg.mid(8).toInt();
		} else if (arg.startsWith("--count=")) {
			count = arg.mid(8).toInt();
		} else if (arg.startsWith("--dive-ref=")) {
			diveRef = arg.mid(11);
		} else if (arg.startsWith("--trip-ref=")) {
			tripRef = arg.mid(11);
		} else if (arg.startsWith("--dc-index=")) {
			dcIndex = arg.mid(11).toInt();
		} else if (arg.startsWith("--width=")) {
			width = arg.mid(8).toInt();
		} else if (arg.startsWith("--height=")) {
			height = arg.mid(9).toInt();
		}
	}

	// Route to appropriate command handler
	int result;
	if (command == "list-dives") {
		result = cmdListDives(config, start, count);
	} else if (command == "get-dive") {
		result = cmdGetDive(config, diveRef);
	} else if (command == "get-trip") {
		result = cmdGetTrip(config, tripRef);
	} else if (command == "get-profile") {
		result = cmdGetProfile(config, diveRef, dcIndex, width, height);
	} else if (command == "get-stats") {
		result = cmdGetStats(config);
	} else {
		fprintf(stderr, "Unknown command: %s\n", qPrintable(command));
		printUsage();
		return 1;
	}

	// Cleanup
	qPref::sync();

	return result;
}
