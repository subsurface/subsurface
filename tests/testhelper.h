// SPDX-License-Identifier: GPL-2.0
#ifndef TESTPARSE_H
#define TESTPARSE_H

#include <QtTest>
#include <sqlite3.h>

class TestHelper : public QObject {
	Q_OBJECT
private slots:
	void initTestCase();
	void recognizeBtAddress();
	void parseNameAddress();
};

#endif
