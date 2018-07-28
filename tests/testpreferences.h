// SPDX-License-Identifier: GPL-2.0
#ifndef TESTPREFERENCES_H
#define TESTPREFERENCES_H

#include <QTest>
#include <functional>

class TestPreferences : public QObject {
	Q_OBJECT
private slots:
	void initTestCase();
	void testPreferences();
};

#endif // TESTPREFERENCES_H
