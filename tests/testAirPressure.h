// SPDX-License-Identifier: GPL-2.0
#ifndef TESTPICTURE_H
#define TESTPICTURE_H

#include "testbase.h"

class TestAirPressure : public TestBase {
	Q_OBJECT
private slots:
	void initTestCase();
	void get_dives();
	void testReadAirPressure();
	void testWriteReadBackAirPressure();
	void testConvertAltitudetoAirPressure();
};

#endif


