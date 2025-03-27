// SPDX-License-Identifier: GPL-2.0
#ifndef TESTQML_H
#define TESTQML_H
#include <QQmlEngine>

#include "testbase.h"

class QMLTestSetup : public TestBase {
	Q_OBJECT

public:
	QMLTestSetup() {}

public slots:
	void qmlEngineAvailable(QQmlEngine *engine);
};

#endif // TESTQML_H
