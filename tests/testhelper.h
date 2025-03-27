// SPDX-License-Identifier: GPL-2.0
#ifndef TESTPARSE_H
#define TESTPARSE_H

#include <sqlite3.h>

#include "testbase.h"

class TestHelper : public TestBase {
	Q_OBJECT
private slots:
	void initTestCase();
	void recognizeBtAddress();
	void parseNameAddress();
};

#endif
