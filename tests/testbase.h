// SPDX-License-Identifier: GPL-2.0
#ifndef TESTBASE_H
#define TESTBASE_H

#include <QtTest>

class TestBase : public QObject {
	Q_OBJECT
protected slots:
	void initTestCase();
};

#endif
