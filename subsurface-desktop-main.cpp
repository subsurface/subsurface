// SPDX-License-Identifier: GPL-2.0
/* main.c */
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "core/dive.h"
#include "core/qt-gui.h"
#include "core/subsurfacestartup.h"
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/tab-widgets/maintab.h"
#include "profile-widget/profilewidget2.h"
#include "desktop-widgets/preferences/preferencesdialog.h"
#include "desktop-widgets/diveplanner.h"
#include "core/color.h"
#include "core/qthelper.h"
#include "core/downloadfromdcthread.h" // for fill_computer_list

#include <QStringList>
#include <QApplication>
#include <QLoggingCategory>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLFunctions>
#include <QQuickWindow>
#include <git2.h>

static bool filesOnCommandLine = false;
static void validateGL();

int main(int argc, char **argv)
{
	int i;
	bool no_filenames = true;
	QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth* = true"));
	QApplication *application = new QApplication(argc, argv);
	(void)application;
	QStringList files;
	QStringList importedFiles;
	QStringList arguments = QCoreApplication::arguments();

	subsurface_console_init();

	const char *default_directory = system_default_directory();
	const char *default_filename = system_default_filename();
	subsurface_mkdir(default_directory);

	for (i = 1; i < arguments.length(); i++) {
		QString a = arguments.at(i);
		if (a.isEmpty())
			continue;
		if (a.at(0) == '-') {
			parse_argument(a.toLocal8Bit().data());
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
	/*
	 * Initialize the random number generator - not really secure as
	 * this is based only on current time, but it should not matter
	 * that much in our context. Moreover this is better than
	 * the constant numbers we used to get before.
	 */
	qsrand(time(NULL));
	setup_system_prefs();
	copy_prefs(&default_prefs, &prefs);
	fill_profile_color();
	fill_computer_list();
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
	free_prefs();
	return 0;
}

bool haveFilesOnCommandLine()
{
	return filesOnCommandLine;
}

#define VALIDATE_GL_PREFIX  "validateGL(): "

void validateGL()
{
	QString quickBackend = qgetenv("QT_QUICK_BACKEND");
	/* an empty QT_QUICK_BACKEND env. variable means OpenGL (default).
	 * only validate OpenGL; for everything else print out and return.
	 * https://doc.qt.io/qt-5/qtquick-visualcanvas-adaptations.html
	 */
	if (!quickBackend.isEmpty()) {
		if (verbose) {
			qDebug() << QStringLiteral(VALIDATE_GL_PREFIX "'QT_QUICK_BACKEND' is set to '%1'. "
				"Skipping validation.").arg(quickBackend).toUtf8().data();
		}
		return;
	}
	GLint verMajor = -1, verMinor;
	const char *glError = NULL;
	QOpenGLContext ctx;
	QOffscreenSurface surface;
	QOpenGLFunctions *func;
	const char *verChar;
	float verFloat;

	surface.setFormat(ctx.format());
	surface.create();
	if (!ctx.create()) {
		glError = "Cannot create OpenGL context";
		goto exit;
	}
	if (verbose)
		qDebug() << QStringLiteral(VALIDATE_GL_PREFIX "created OpenGLContext.").toUtf8().data();
	ctx.makeCurrent(&surface);
	func = ctx.functions();
	if (!func) {
		glError = "Cannot obtain QOpenGLFunctions";
		goto exit;
	}
	if (verbose)
		qDebug() << QStringLiteral(VALIDATE_GL_PREFIX "obtained QOpenGLFunctions.").toUtf8().data();
	// detect version for legacy profiles
	verChar = (const char *)func->glGetString(GL_VERSION);
	if (verChar) {
		// detect GLES, show a warning and return early as we don't handle it's versioning
		if (strstr(verChar, " ES ") != NULL) {
			 qWarning() << QStringLiteral(VALIDATE_GL_PREFIX "WARNING: Detected OpenGL ES!\n"
			 "Attempting to run with the available profile!\n"
			 "If this fails try manually setting the environment variable\n"
			 "'QT_QUICK_BACKEND' with the value of 'software'\n"
			 "before running Subsurface!\n").toUtf8().data();
			 return;
		}
		if (sscanf(verChar, "%f", &verFloat) == 1) {
			verMajor = (GLint)verFloat;
			verMinor = (GLint)roundf((verFloat - verMajor) * 10.f);
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
		qDebug() << QStringLiteral(VALIDATE_GL_PREFIX "detected OpenGL version %1.%2.").arg(verMajor).arg(verMinor).toUtf8().data();
	if (verMajor * 10 + verMinor < 21) { // set 2.1 as the minimal version
		glError = "OpenGL 2.1 or later is required";
		goto exit;
	}

exit:
	ctx.makeCurrent(NULL);
	surface.destroy();
	if (glError) {
#if QT_VERSION < QT_VERSION_CHECK(5, 8, 0)
		qWarning() << QStringLiteral(VALIDATE_GL_PREFIX "ERROR: %1.\n"
			"Cannot automatically fallback to a software renderer!\n"
			"Set the environment variable 'QT_QUICK_BACKEND' with the value of 'software'\n"
			"before running Subsurface!\n").arg(glError).toUtf8().data();
		exit(0);
#else
		qWarning() << QStringLiteral(VALIDATE_GL_PREFIX "WARNING: %1. Using a software renderer!").arg(glError).toUtf8().data();
		QQuickWindow::setSceneGraphBackend(QSGRendererInterface::Software);
#endif
	}
}
