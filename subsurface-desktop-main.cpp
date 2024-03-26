// SPDX-License-Identifier: GPL-2.0
/* main.c */
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "core/downloadfromdcthread.h" // for fill_computer_list
#include "core/divelog.h"
#include "core/errorhelper.h"
#include "core/parse.h"
#include "core/qt-gui.h"
#include "core/qthelper.h"
#include "core/subsurfacestartup.h"
#include "core/subsurface-string.h"
#include "core/settings/qPref.h"
#include "core/tag.h"
#include "desktop-widgets/mainwindow.h"
#include "core/checkcloudconnection.h"

#include <QApplication>
#include <QLoggingCategory>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QQuickWindow>
#include <QStringList>
#include <git2.h>

static void validateGL();
static void messageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg);

int main(int argc, char **argv)
{
	subsurface_console_init();
	qInstallMessageHandler(messageHandler);
	if (verbose) /* print the version if the Win32 console_init() code enabled verbose. */
		print_version();

	bool no_filenames = true;
	QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth* = true"));
	std::unique_ptr<QApplication> app(new QApplication(argc, argv));
	std::vector<std::string> files;
	std::vector<std::string> importedFiles;
	QStringList arguments = QCoreApplication::arguments();

	const char *default_directory = system_default_directory();
	subsurface_mkdir(default_directory);

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
	if (subsurface_user_is_root() && !force_root) {
		printf("You are running Subsurface as root. This is not recommended.\n");
		printf("If you insist to do so, run with option --allow_run_as_root.\n");
		exit(0);
	}
	validateGL();
#if !LIBGIT2_VER_MAJOR && LIBGIT2_VER_MINOR < 22
	git_threads_init();
#else
	git_libgit2_init();
#endif
	setup_system_prefs();
	copy_prefs(&default_prefs, &prefs);
	CheckCloudConnection ccc;
	ccc.pickServer();
	fill_computer_list();
	reset_tank_info_table(&tank_info_table);
	parse_xml_init();
	taglist_init_global();
	init_ui();
	if (no_filenames) {
		if (prefs.default_file_behavior == LOCAL_DEFAULT_FILE) {
			if (!empty_string(prefs.default_filename))
				files.emplace_back(prefs.default_filename ? prefs.default_filename : "");
		} else if (prefs.default_file_behavior == CLOUD_DEFAULT_FILE) {
			auto cloudURL = getCloudURL();
			if (cloudURL)
				files.push_back(*cloudURL);
		}
	}
	MainWindow *m = MainWindow::instance();
	if (verbose && !files.empty())
		report_info("loading dive data from: %s", join(files, std::string(", ")).c_str());
	m->loadFiles(files);
	if (verbose && !importedFiles.empty())
		report_info("importing dive data from %s", join(importedFiles, std::string(", ")).c_str());
	m->importFiles(importedFiles);

	if (verbose > 0)
		print_files();
	if (!quit)
		run_ui();
	exit_ui();
	clear_divelog(&divelog);
	parse_xml_exit();
	subsurface_console_exit();

	// Sync struct preferences to disk
	qPref::sync();

	free_prefs();
	clear_tank_info_table(&tank_info_table);
	return 0;
}

#define VALIDATE_GL_PREFIX "validateGL(): "

void validateGL()
{
	std::string quickBackend = qgetenv("QT_QUICK_BACKEND").toStdString();
	/* on macOS with Qt6 (maybe others), things only work with the software backend */
#if defined(Q_OS_MACOS) && QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	if (quickBackend.empty()) {
		quickBackend = "software";
		qputenv("QT_QUICK_BACKEND", "software");
	}
#endif
	/* an empty QT_QUICK_BACKEND env. variable means OpenGL (default).
	 * only validate OpenGL; for everything else print out and return.
	 * https://doc.qt.io/qt-5/qtquick-visualcanvas-adaptations.html
	 */
	if (!quickBackend.empty()) {
		if (verbose) {
			report_info(VALIDATE_GL_PREFIX
				    "'QT_QUICK_BACKEND' is set to '%s'. "
				    "Skipping validation.", quickBackend.c_str());
		}
		return;
	}
	GLint verMajor = -1, verMinor;
	const char *glError = NULL;
	QOpenGLContext ctx;
	QOffscreenSurface surface;
	QOpenGLFunctions *func;
	const char *verChar;

	surface.setFormat(ctx.format());
	surface.create();
	if (!ctx.create()) {
		glError = "Cannot create OpenGL context";
		goto exit;
	}
	if (verbose)
		report_info(VALIDATE_GL_PREFIX "created OpenGLContext.");
	ctx.makeCurrent(&surface);
	func = ctx.functions();
	if (!func) {
		glError = "Cannot obtain QOpenGLFunctions";
		goto exit;
	}
	if (verbose)
		report_info(VALIDATE_GL_PREFIX "obtained QOpenGLFunctions.");
	// detect version for legacy profiles
	verChar = (const char *)func->glGetString(GL_VERSION);
	if (verChar) {
		// detect GLES, show a warning and return early as we don't handle it's versioning
		if (strstr(verChar, " ES ") != NULL) {
			report_error(VALIDATE_GL_PREFIX
				     "WARNING: Detected OpenGL ES!\n"
				     "Attempting to run with the available profile!\n"
				     "If this fails try manually setting the environment variable\n"
				     "'QT_QUICK_BACKEND' with the value of 'software'\n"
				     "before running Subsurface!\n");
			return;
		}
		int min, maj;
		if (sscanf(verChar, "%d.%d", &maj, &min) == 2) {
			verMajor = (GLint)maj;
			verMinor = (GLint)min;
		}
	}
	// attempt to detect version using the newer API
	if (verMajor == -1) {
		func->glGetIntegerv(GL_MAJOR_VERSION, &verMajor);
		func->glGetIntegerv(GL_MINOR_VERSION, &verMinor);
	}
	if (verMajor == -1) {
		glError = "Cannot detect OpenGL version";
		goto exit;
	}
	if (verbose)
		report_info(VALIDATE_GL_PREFIX "detected OpenGL version %d.%d.", verMajor, verMinor);
	if (verMajor * 10 + verMinor < 21) { // set 2.1 as the minimal version
		glError = "OpenGL 2.1 or later is required";
		goto exit;
	}

exit:
	ctx.makeCurrent(NULL);
	surface.destroy();
	if (glError) {
		report_error(VALIDATE_GL_PREFIX "WARNING: %s. Using a software renderer!", glError);
		QQuickWindow::setSceneGraphBackend("software");
	}
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
