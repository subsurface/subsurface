// SPDX-License-Identifier: GPL-2.0

#ifndef TESTPROFILE_H
#define TESTPROFILE_H

#include "testbase.h"

class TestProfile : public TestBase {
	Q_OBJECT
private slots:
	void init();
	void testProfileExport();
	void testProfileExportVPMB();
};

#endif
