// SPDX-License-Identifier: GPL-2.0
#include "testqml.h"
#include "core/settings/qPref.h"

#include <QtQuickTest>

// main loosely copied from QUICK_TEST_MAIN_WITH_SETUP macro
int main(int argc, char **argv)
{
//#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
#ifndef THIS_IS_REPAIRED
	return 0;
#else
	QTEST_ADD_GPU_BLACKLIST_SUPPORT
	QTEST_SET_MAIN_SOURCE_PATH
	QMLTestSetup setup;

	// register C++ types and classes (but not objects)
	qPref::registerQML(NULL);

	return quick_test_main_with_setup(argc, argv, "TestQML", nullptr, &setup);
#endif //QT_VERSION
}

void QMLTestSetup::qmlEngineAvailable(QQmlEngine *engine)
{
	// register C++ objects (but not types and classes)
	qPref::registerQML(engine);
};

