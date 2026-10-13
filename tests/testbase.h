// SPDX-License-Identifier: GPL-2.0
#ifndef TESTBASE_H
#define TESTBASE_H

#include <QtTest>

#include <string>

class TestBase : public QObject {
	Q_OBJECT
protected slots:
	void initTestCase();
public:
	// Exposed (rather than kept file-local) so that a test which needs to
	// provoke an expected, by-design report_error() call can temporarily
	// install a different callback via set_error_cb() and then restore
	// this one afterwards.
	static void failOnError(std::string error);
};

#endif
