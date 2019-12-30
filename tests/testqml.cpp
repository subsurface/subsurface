// SPDX-License-Identifier: GPL-2.0
#include <QtQuickTest>
#include <QQmlEngine>
#include <QQmlContext>
#include "core/settings/qPref.h"


class TestQMLSetup : public QObject
{
	Q_OBJECT

public:
	TestQMLSetup() {}

public slots:
//	void applicationAvailable()
//	{
//		// register C++ types and classes (but not objects)
//	}
	void qmlEngineAvailable(QQmlEngine *engine)
	{
		// register C++ objects
		qPref::registerQML(NULL);
	}
};

QUICK_TEST_MAIN_WITH_SETUP(TestQML, TestQMLSetup)

#include "testqml.moc"
