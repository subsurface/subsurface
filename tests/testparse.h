// SPDX-License-Identifier: GPL-2.0
#ifndef TESTPARSE_H
#define TESTPARSE_H

#include <sqlite3.h>

#include "testbase.h"

class TestParse : public TestBase {
	Q_OBJECT
private slots:
	void initTestCase();
	void init();
	void cleanup();

	int parseCSV(int, std::string);
	int parseDivingLog();
	int parseV2NoQuestion();
	int parseV3();
	void testParse();
	void testParseTankSensors();

	void testParseDM4();
	void testParseDM5();
	void testParseHUDC();
	void testParseNewFormat();
	void testParseDLD();
	void testParseMerge();
	void testParseMergeTankSensors();

	int parseCSVmanual(int, std::string);
	void exportSubsurfaceCSV();
	void exportCSVDiveDetails();
	int parseCSVprofile(int, std::string);
	void exportCSVDiveProfile();
	void exportUDDF();

	void parseDL7();

	void importApdInspirationUddf();
private:
	sqlite3 *_sqlite3_handle = NULL;
};

#endif
