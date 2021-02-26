// SPDX-License-Identifier: GPL-2.0

#ifndef TESTPROFILE_H
#define TESTPROFILE_H

#include <QtTest>

class TestProfile : public QObject {
	Q_OBJECT
private slots:
	void testProfileExport();
	void testProfileExportVPMB();
};

#endif
