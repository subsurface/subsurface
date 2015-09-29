#include "testparse.h"
#include "dive.h"
#include "file.h"
#include "divelist.h"
#include <QTextStream>

char *intdup(int index)
{
	char tmpbuf[21];

	snprintf(tmpbuf, sizeof(tmpbuf) - 2, "%d", index);
	tmpbuf[20] = 0;
	return strdup(tmpbuf);
}

void TestParse::testParseCSV()
{
	// some basic file parsing tests
	//
	// CSV import should work
	verbose = 1;
	char *params[55];
	int pnr = 0;

	params[pnr++] = strdup(strdup("numberField"));
	params[pnr++] = intdup(0);
	params[pnr++] = strdup("dateField");
	params[pnr++] = intdup(1);
	params[pnr++] = strdup("timeField");
	params[pnr++] = intdup(2);
	params[pnr++] = strdup("durationField");
	params[pnr++] = intdup(3);
	params[pnr++] = strdup("locationField");
	params[pnr++] = intdup(-1);
	params[pnr++] = strdup("gpsField");
	params[pnr++] = intdup(-1);
	params[pnr++] = strdup("maxDepthField");
	params[pnr++] = intdup(4);
	params[pnr++] = strdup("meanDepthField");
	params[pnr++] = intdup(5);
	params[pnr++] = strdup("divemasterField");
	params[pnr++] = intdup(-1);
	params[pnr++] = strdup("buddyField");
	params[pnr++] = intdup(6);
	params[pnr++] = strdup("suitField");
	params[pnr++] = intdup(7);
	params[pnr++] = strdup("notesField");
	params[pnr++] = intdup(-1);
	params[pnr++] = strdup("weightField");
	params[pnr++] = intdup(-1);
	params[pnr++] = strdup("tagsField");
	params[pnr++] = intdup(-1);
	params[pnr++] = strdup("separatorIndex");
	params[pnr++] = intdup(0);
	params[pnr++] = strdup("units");
	params[pnr++] = intdup(0);
	params[pnr++] = strdup("datefmt");
	params[pnr++] = intdup(1);
	params[pnr++] = strdup("durationfmt");
	params[pnr++] = intdup(2);
	params[pnr++] = strdup("cylindersizeField");
	params[pnr++] = intdup(-1);
	params[pnr++] = strdup("startpressureField");
	params[pnr++] = intdup(-1);
	params[pnr++] = strdup("endpressureField");
	params[pnr++] = intdup(-1);
	params[pnr++] = strdup("o2Field");
	params[pnr++] = intdup(-1);
	params[pnr++] = strdup("heField");
	params[pnr++] = intdup(-1);
	params[pnr++] = strdup("airtempField");
	params[pnr++] = intdup(-1);
	params[pnr++] = strdup("watertempField");
	params[pnr++] = intdup(-1);
	params[pnr++] = NULL;

	QCOMPARE(parse_manual_file(SUBSURFACE_SOURCE "/dives/test41.csv", params, pnr - 1), 0);
	fprintf(stderr, "number of dives %d \n", dive_table.nr);
}

void TestParse::testParseDivingLog()
{
	// Parsing of DivingLog import from SQLite database
	sqlite3 *handle;

	struct dive_site *ds = alloc_or_get_dive_site(0xdeadbeef);
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
	char *params[37];
	int pnr = 0;

	params[pnr++] = strdup("timeField");
	params[pnr++] = intdup(0);
	params[pnr++] = strdup("depthField");
	params[pnr++] = intdup(1);
	params[pnr++] = strdup("tempField");
	params[pnr++] = intdup(5);
	params[pnr++] = strdup("po2Field");
	params[pnr++] = intdup(-1);
	params[pnr++] = strdup("o2sensor1Field");
	params[pnr++] = intdup(-1);
	params[pnr++] = strdup("o2sensor2Field");
	params[pnr++] = intdup(-1);
	params[pnr++] = strdup("o2sensor3Field");
	params[pnr++] = intdup(-1);
	params[pnr++] = strdup("cnsField");
	params[pnr++] = intdup(-1);
	params[pnr++] = strdup("ndlField");
	params[pnr++] = intdup(2);
	params[pnr++] = strdup("ttsField");
	params[pnr++] = intdup(-1);
	params[pnr++] = strdup("stopdepthField");
	params[pnr++] = intdup(-1);
	params[pnr++] = strdup("pressureField");
	params[pnr++] = intdup(-1);
	params[pnr++] = strdup("setpointFiend");
	params[pnr++] = intdup(-1);
	params[pnr++] = strdup("separatorIndex");
	params[pnr++] = intdup(2);
	params[pnr++] = strdup("units");
	params[pnr++] = intdup(0);
	params[pnr++] = strdup("hw");
	params[pnr++] = strdup("\"DC text\"");
	params[pnr++] = NULL;

	QCOMPARE(parse_csv_file(SUBSURFACE_SOURCE "/dives/TestDiveSeabearHUDC.csv",
				params, pnr - 1, "csv"), 0);

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

		char *params[40];
		int pnr = 0;

		params[pnr++] = strdup("timeField");
		params[pnr++] = intdup(headers.indexOf(tr("Sample time")));
		params[pnr++] = strdup("depthField");
		params[pnr++] = intdup(headers.indexOf(tr("Sample depth")));
		params[pnr++] = strdup("tempField");
		params[pnr++] = intdup(headers.indexOf(tr("Sample temperature")));
		params[pnr++] = strdup("po2Field");
		params[pnr++] = intdup(headers.indexOf(tr("Sample pO₂")));
		params[pnr++] = strdup("o2sensor1Field");
		params[pnr++] = intdup(headers.indexOf(tr("Sample sensor1 pO₂")));
		params[pnr++] = strdup("o2sensor2Field");
		params[pnr++] = intdup(headers.indexOf(tr("Sample sensor2 pO₂")));
		params[pnr++] = strdup("o2sensor3Field");
		params[pnr++] = intdup(headers.indexOf(tr("Sample sensor3 pO₂")));
		params[pnr++] = strdup("cnsField");
		params[pnr++] = intdup(headers.indexOf(tr("Sample CNS")));
		params[pnr++] = strdup("ndlField");
		params[pnr++] = intdup(headers.indexOf(tr("Sample NDL")));
		params[pnr++] = strdup("ttsField");
		params[pnr++] = intdup(headers.indexOf(tr("Sample TTS")));
		params[pnr++] = strdup("stopdepthField");
		params[pnr++] = intdup(headers.indexOf(tr("Sample stopdepth")));
		params[pnr++] = strdup("pressureField");
		params[pnr++] = intdup(headers.indexOf(tr("Sample pressure")));
		params[pnr++] = strdup("setpointFiend");
		params[pnr++] = intdup(-1);
		params[pnr++] = strdup("separatorIndex");
		params[pnr++] = intdup(2);
		params[pnr++] = strdup("units");
		params[pnr++] = intdup(0);
		params[pnr++] = strdup("delta");
		params[pnr++] = strdup(delta.toUtf8().data());
		params[pnr++] = NULL;

		QCOMPARE(parse_seabear_csv_file(file.toUtf8().data(),
					params, pnr - 1, "csv"), 0);

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

void TestParse::testParseDLD()
{
	struct memblock mem;
	int ret, success = 0;
	QString filename = SUBSURFACE_SOURCE "/dives/TestDiveDivelogsDE.DLD";

	QVERIFY(readfile(filename.toLatin1().data(), &mem) > 0);
	QVERIFY(try_to_open_zip(filename.toLatin1().data(), &mem) > 0);

	fprintf(stderr, "number of dives from DLD: %d \n", dive_table.nr);
}

void TestParse::testParseCompareDLDOutput()
{
	/*
	 * DC is not cleared from previous tests with the
	 * clear_dive_file_data(), so we do have an additional DC nick
	 * name field on the log.
	 */

	QCOMPARE(save_dives("./testdldout.ssrf"), 0);
	QFile org(SUBSURFACE_SOURCE "/dives/TestDiveDivelogsDE.xml");
	org.open(QFile::ReadOnly);
	QFile out("./testdldout.ssrf");
	out.open(QFile::ReadOnly);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);
	clear_dive_file_data();
}

QTEST_MAIN(TestParse)
