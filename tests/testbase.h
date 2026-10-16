// SPDX-License-Identifier: GPL-2.0
#ifndef TESTBASE_H
#define TESTBASE_H

#include <QtTest>

class TestBase : public QObject {
	Q_OBJECT
public:
	static TestBase* instance();
	static void failOnError(const std::string& error);
	static bool skipErrors;
protected:
	void setSkipErrors(bool enabled);
protected slots:
	void initTestCase();
};

#endif
