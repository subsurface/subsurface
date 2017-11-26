// SPDX-License-Identifier: GPL-2.0
#ifndef TESTPARSE_H
#define TESTPARSE_H

#include <QtTest>
#include <sqlite3.h>

class TestParse : public QObject{
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

	void testParseDM4();
	void testParseDM5();
	void testParseHUDC();
	void testParseNewFormat();
	void testParseDLD();
	void testParseMerge();

	int parseCSVmanual(int, std::string);
	void exportCSVDiveDetails();
	int parseCSVprofile(int, std::string);
	void exportCSVDiveProfile();
	void exportUDDF();
	void testExport();

private:
	sqlite3 *_sqlite3_handle = NULL;
};

#endif
