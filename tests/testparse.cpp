#include "testparse.h"
#include "dive.h"
#include <QTextStream>

void TestParse::testParseCSV()
{
	// some basic file parsing tests
	//
	// even with the V2 question not shown, CSV import should work
	v2_question_shown = false;
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

void TestParse::testParseV2NoQuestion()
{
	// but parsing of a V2 file should fail
	v2_question_shown = false;
	QCOMPARE(parse_file(SUBSURFACE_SOURCE "/dives/test40.xml"), -1);
}

void TestParse::testParseV3()
{
	// while parsing of a V3 files should succeed
	v2_question_shown = false;
	QCOMPARE(parse_file(SUBSURFACE_SOURCE "/dives/test42.xml"), 0);
}

void TestParse::testParseV2YesQuestion()
{
	// once we claim to have shown the V2 question, parsing the V2 file should work as well
	v2_question_shown = true;
	QCOMPARE(parse_file(SUBSURFACE_SOURCE "/dives/test40.xml"), 0);
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
}

QTEST_MAIN(TestParse)
