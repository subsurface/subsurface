// SPDX-License-Identifier: GPL-2.0
#ifndef TESTPARSEPERFORMANCE_H
#define TESTPARSEPERFORMANCE_H

#include <QtTest>

class TestParsePerformance : public QObject {
	Q_OBJECT
private slots:
	void initTestCase();
	void init();
	void cleanup();

	void parseSsrf();
	void parseGit();
};

#endif
