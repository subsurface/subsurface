#include "testparse.h"
#include "dive.h"
#include "file.h"
#include "divelist.h"
#include <QTextStream>

void TestParse::testParseCSV()
{
	// some basic file parsing tests
	//
	// CSV import should work
	verbose = 1;
	QCOMPARE(parse_manual_file(SUBSURFACE_SOURCE "/dives/test41.csv",
				   0, // tab separator
				   0, // metric units
				   1, // mm/dd/yyyy
				   2, // min:sec
				   0, 1, 2, 3, -1, -1, 4, 5, // Dive #, date, time, duration, maxdepth, avgdepth
				   -1, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1), 0); // buddy, suit
	fprintf(stderr, "number of dives %d \n", dive_table.nr);
}

void TestParse::testParseDivingLog()
{
	// Parsing of DivingLog import from SQLite database
	sqlite3 *handle;

	struct dive_site *ds = alloc_dive_site(0xdeadbeef);
	ds->name = copy_string("Suomi -  - Hälvälä");

	QCOMPARE(sqlite3_open(SUBSURFACE_SOURCE "/dives/TestDivingLog4.1.1.sql", &handle), 0);
	QCOMPARE(parse_divinglog_buffer(handle, 0, 0, 0, &dive_table), 0);

	sqlite3_close(handle);
}

void TestParse::testParseV2NoQuestion()
{
	// parsing of a V2 file should work
	QCOMPARE(parse_file(SUBSURFACE_SOURCE "/dives/test40.xml"), 0);
}

void TestParse::testParseV3()
{
	// parsing of a V3 files should succeed
	QCOMPARE(parse_file(SUBSURFACE_SOURCE "/dives/test42.xml"), 0);
}

void TestParse::testParseCompareOutput()
{
	QCOMPARE(save_dives("./testout.ssrf"), 0);
	QFile org(SUBSURFACE_SOURCE "/dives/test40-42.xml");
	org.open(QFile::ReadOnly);
	QFile out("./testout.ssrf");
	out.open(QFile::ReadOnly);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);
	clear_dive_file_data();
}

void TestParse::testParseDM4()
{
	sqlite3 *handle;

	QCOMPARE(sqlite3_open(SUBSURFACE_SOURCE "/dives/TestDiveDM4.db", &handle), 0);
	QCOMPARE(parse_dm4_buffer(handle, 0, 0, 0, &dive_table), 0);

	sqlite3_close(handle);
}

void TestParse::testParseCompareDM4Output()
{
	QCOMPARE(save_dives("./testdm4out.ssrf"), 0);
	QFile org(SUBSURFACE_SOURCE "/dives/TestDiveDM4.xml");
	org.open(QFile::ReadOnly);
	QFile out("./testdm4out.ssrf");
	out.open(QFile::ReadOnly);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);
	clear_dive_file_data();
}

void TestParse::testParseHUDC()
{
	QCOMPARE(parse_csv_file(SUBSURFACE_SOURCE "/dives/TestDiveSeabearHUDC.csv",
				0,  // sample time
				1,  // sample depth
				5,  // sample temperature
				-1, // sample pO₂
				-1, // sample sensor1 pO₂
				-1, // sample sensor2 pO₂
				-1, // sample sensor3 pO₂
				-1, // sample cns
				2,  // sample ndl
				-1,  // sample tts
				-1, // sample stopdepth
				-1, // sample pressure
				-1, // smaple setpoint
				2,  // separator index
				"csv", // XSLT template
				0,  // units
				"\"DC text\""), 0);

	/*
	 * CSV import uses time and date stamps relative to current
	 * time, thus we need to use a static (random) timestamp
	 */

	struct dive *dive = dive_table.dives[dive_table.nr - 1];
	dive->when = 1255152761;
	dive->dc.when = 1255152761;
}

void TestParse::testParseCompareHUDCOutput()
{
	QCOMPARE(save_dives("./testhudcout.ssrf"), 0);
	QFile org(SUBSURFACE_SOURCE "/dives/TestDiveSeabearHUDC.xml");
	org.open(QFile::ReadOnly);
	QFile out("./testhudcout.ssrf");
	out.open(QFile::ReadOnly);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);
	clear_dive_file_data();
}

QTEST_MAIN(TestParse)
