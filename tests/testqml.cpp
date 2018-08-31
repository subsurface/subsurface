// SPDX-License-Identifier: GPL-2.0
#include <QApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QtQuickTest>
#include <QtTest>

#include "core/qt-gui.h"
#include "core/settings/qPref.h"

// this is the content of QUICK_TEST_MAIN amended with
// registration of ssrf classes
int main(int argc, char **argv)
{
	// QML testing is not supported in the oldest Qt versions we support
	// if running with Qt version less than 5.10 then skip test
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
	QTEST_ADD_GPU_BLACKLIST_SUPPORT
	QTEST_SET_MAIN_SOURCE_PATH

	// check that qPref classes exists
	qPref::instance();
	qPrefDisplay::instance();

	// prepare Qt application
	new QApplication(argc, argv);

	// check that we have a directory
	if (argc < 2) {
		qDebug() << "ERROR: missing tst_* directory";
		return -1;
	}
	// save tst_dir and pass rest to Qt
	const char *tst_dir = argv[1];
	for (int i = 1; i < argc; i++)
		argv[i] = argv[i + 1];
	argc--;

	// Register types
	qPref::instance()->registerQML();

	// Run all tst_*.qml files
	return quick_test_main(argc, argv, "TestQML", tst_dir);
#else
	return 0;
#endif
}
