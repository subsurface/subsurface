// SPDX-License-Identifier: GPL-2.0
#ifndef TESTQPREFDISPLAY_H
#define TESTQPREFDISPLAY_H

#include <QTest>
#include <functional>

class TestQPrefDisplay : public QObject
{
	Q_OBJECT
private slots:
	void initTestCase();
	void test1();
};

#endif // TESTQPREFDISPLAY_H
