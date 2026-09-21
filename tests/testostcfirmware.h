// SPDX-License-Identifier: GPL-2.0
#ifndef TESTOSTCFIRMWARE_H
#define TESTOSTCFIRMWARE_H

#include "testbase.h"

class TestOstcFirmware : public TestBase {
	Q_OBJECT
private slots:
	void firmwareVersionComponentCounts();
	void detectedOstc4RejectsOstc3Version();
};

#endif
