// SPDX-License-Identifier: GPL-2.0
#ifndef TESTQPREFMEDIA_H
#define TESTQPREFMEDIA_H

#include "testbase.h"

class TestQPrefMedia : public TestBase {
	Q_OBJECT

private slots:
	void initTestCase();
	void test_struct_get();
	void test_set_struct();
	void test_set_load_struct();
	void test_struct_disk();
	void test_signals();
};

#endif // TESTQPREFMEDIA_H
