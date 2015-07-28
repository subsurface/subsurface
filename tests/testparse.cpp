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

void TestParse::testParseNewFormat()
{
	struct dive *dive;
	QDir dir;
	QStringList filter;
	QString firstLine;
	QStringList files;

	/*
	 * Set the directory location and file filter for H3 CSV files.
	 */

	dir = QString::fromLatin1(SUBSURFACE_SOURCE "/dives");
	filter << "TestDiveSeabearH3*.csv";
	filter << "TestDiveSeabearT1*.csv";
	files = dir.entryList(filter, QDir::Files);

	/*
	 * Parse all files found matching the filter.
	 */

	for (int i = 0; i < files.size(); ++i) {
		QString delta;
		QStringList currColumns;
		QStringList headers;
		QString file = QString::fromLatin1(SUBSURFACE_SOURCE "/dives/").append(files.at(i));
		QFile f(file);

		/*
		 * Parse the sample interval if available from CSV
		 * header.
		 */

		f.open(QFile::ReadOnly);
		while ((firstLine = f.readLine().trimmed()).length() > 0 && !f.atEnd()) {
			if (firstLine.contains("//Log interval: "))
				delta = firstLine.remove(QString::fromLatin1("//Log interval: ")).trimmed().remove(QString::fromLatin1(" s"));
		}
		firstLine = f.readLine().trimmed();

		/*
		 * Parse the field configuration from the CSV header.
		 */

		currColumns = firstLine.split(';');
		Q_FOREACH (QString columnText, currColumns) {
			if (columnText == "Time") {
				headers.append("Sample time");
			} else if (columnText == "Depth") {
				headers.append("Sample depth");
			} else if (columnText == "Temperature") {
				headers.append("Sample temperature");
			} else if (columnText == "NDT") {
				headers.append("Sample NDL");
			} else if (columnText == "TTS") {
				headers.append("Sample TTS");
			} else if (columnText == "pO2_1") {
				headers.append("Sample sensor1 pO₂");
			} else if (columnText == "pO2_2") {
				headers.append("Sample sensor2 pO₂");
			} else if (columnText == "pO2_3") {
				headers.append("Sample sensor3 pO₂");
			} else if (columnText == "Ceiling") {
				headers.append("Sample ceiling");
			} else if (columnText == "Tank pressure") {
				headers.append("Sample pressure");
			} else {
				// We do not know about this value
				qDebug() << "Seabear import found an un-handled field: " << columnText;
				headers.append("");
			}
			f.close();
		}

		/*
		 * Validate the parsing routine returns success.
		 */

		QCOMPARE(parse_seabear_csv_file(file.toUtf8().data(),
					headers.indexOf(tr("Sample time")),
					headers.indexOf(tr("Sample depth")),
					headers.indexOf(tr("Sample temperature")),
					headers.indexOf(tr("Sample pO₂")),
					headers.indexOf(tr("Sample sensor1 pO₂")),
					headers.indexOf(tr("Sample sensor2 pO₂")),
					headers.indexOf(tr("Sample sensor3 pO₂")),
					headers.indexOf(tr("Sample CNS")),
					headers.indexOf(tr("Sample NDL")),
					headers.indexOf(tr("Sample TTS")),
					headers.indexOf(tr("Sample stopdepth")),
					headers.indexOf(tr("Sample pressure")),
					2,
					"csv",
					0,
					delta.toUtf8().data(),
					""
					), 0);

		/*
		 * Set artificial but static dive times so the result
		 * can be compared to saved one.
		 */

		dive = dive_table.dives[dive_table.nr - 1];
		dive->when = 1255152761 + 7200 * i;
		dive->dc.when = 1255152761 + 7200 * i;
	}

	fprintf(stderr, "number of dives %d \n", dive_table.nr);
}

void TestParse::testParseCompareNewFormatOutput()
{
	QCOMPARE(save_dives("./testsbnewout.ssrf"), 0);
	QFile org(SUBSURFACE_SOURCE "/dives/TestDiveSeabearNewFormat.xml");
	org.open(QFile::ReadOnly);
	QFile out("./testsbnewout.ssrf");
	out.open(QFile::ReadOnly);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);
	clear_dive_file_data();
}

QTEST_MAIN(TestParse)
