// SPDX-License-Identifier: GPL-2.0
/* main.c */
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "core/downloadfromdcthread.h" // for fill_computer_list
#include "core/errorhelper.h"
#include "core/parse.h"
#include "core/qt-gui.h"
#include "core/qthelper.h"
#include "core/subsurfacestartup.h"
#include "core/settings/qPref.h"
#include "core/tag.h"
#include "desktop-widgets/mainwindow.h"

#include <QApplication>
#include <QLoggingCategory>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QQuickWindow>
#include <QStringList>
#include <git2.h>

static bool filesOnCommandLine = false;
static void validateGL();
static void messageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg);

int main(int argc, char **argv)
{
	subsurface_console_init();
	qInstallMessageHandler(messageHandler);
	if (verbose) /* print the version if the Win32 console_init() code enabled verbose. */
		print_version();

	int i;
	bool no_filenames = true;
	QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth* = true"));
	std::unique_ptr<QApplication> app(new QApplication(argc, argv));
	QStringList files;
	QStringList importedFiles;
	QStringList arguments = QCoreApplication::arguments();

	const char *default_directory = system_default_directory();
	const char *default_filename = system_default_filename();
	subsurface_mkdir(default_directory);

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
	fill_computer_list();
	reset_tank_info_table(&tank_info_table);
	parse_xml_init();
	taglist_init_global();
	init_ui();
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
	MainWindow *m = MainWindow::instance();
	filesOnCommandLine = !files.isEmpty() || !importedFiles.isEmpty();
	if (verbose && !files.isEmpty())
		qDebug() << "loading dive data from" << files;
	m->loadFiles(files);
	if (verbose && !importedFiles.isEmpty())
		qDebug() << "importing dive data from" << importedFiles;
	m->importFiles(importedFiles);

	if (verbose > 0)
		print_files();
	if (!quit)
		run_ui();
	exit_ui();
	taglist_free(g_tag_list);
	parse_xml_exit();
	free((void *)default_directory);
	free((void *)default_filename);
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
	QString quickBackend = qgetenv("QT_QUICK_BACKEND");
	/* an empty QT_QUICK_BACKEND env. variable means OpenGL (default).
	 * only validate OpenGL; for everything else print out and return.
	 * https://doc.qt.io/qt-5/qtquick-visualcanvas-adaptations.html
	 */
	if (!quickBackend.isEmpty()) {
		if (verbose) {
			qDebug() << QStringLiteral(VALIDATE_GL_PREFIX
						   "'QT_QUICK_BACKEND' is set to '%1'. "
						   "Skipping validation.")
					    .arg(quickBackend);
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
		qDebug() << QStringLiteral(VALIDATE_GL_PREFIX "created OpenGLContext.");
	ctx.makeCurrent(&surface);
	func = ctx.functions();
	if (!func) {
		glError = "Cannot obtain QOpenGLFunctions";
		goto exit;
	}
	if (verbose)
		qDebug() << QStringLiteral(VALIDATE_GL_PREFIX "obtained QOpenGLFunctions.");
	// detect version for legacy profiles
	verChar = (const char *)func->glGetString(GL_VERSION);
	if (verChar) {
		// detect GLES, show a warning and return early as we don't handle it's versioning
		if (strstr(verChar, " ES ") != NULL) {
			qWarning() << QStringLiteral(VALIDATE_GL_PREFIX
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
		qDebug() << QStringLiteral(VALIDATE_GL_PREFIX "detected OpenGL version %1.%2.").arg(verMajor).arg(verMinor);
	if (verMajor * 10 + verMinor < 21) { // set 2.1 as the minimal version
		glError = "OpenGL 2.1 or later is required";
		goto exit;
	}

exit:
	ctx.makeCurrent(NULL);
	surface.destroy();
	if (glError) {
		qWarning() << QStringLiteral(VALIDATE_GL_PREFIX "WARNING: %1. Using a software renderer!").arg(glError);
		QQuickWindow::setSceneGraphBackend(QSGRendererInterface::Software);
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
