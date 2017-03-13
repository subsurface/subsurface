#include "testparse.h"
#include "core/dive.h"
#include "core/file.h"
#include "core/divelist.h"
#include <QTextStream>

/* We have to use a macro since QCOMPARE
 * can only be called from a test method
 * invoked by the QTest framework
 */
#define FILE_COMPARE(actual, expected)				\
	QFile org(expected);					\
	org.open(QFile::ReadOnly);				\
	QFile out(actual);					\
	out.open(QFile::ReadOnly);				\
	QTextStream orgS(&org);					\
	QTextStream outS(&out);					\
	QStringList readin = orgS.readAll().split("\n");	\
	QStringList written = outS.readAll().split("\n");	\
	while(readin.size() && written.size()){			\
		QCOMPARE(written.takeFirst().trimmed(),		\
			readin.takeFirst().trimmed());		\
	}							\

void TestParse::initTestCase()
{
	/* we need to manually tell that the resource exists, because we are using it as library. */
	Q_INIT_RESOURCE(subsurface);
}

void TestParse::init()
{
	_sqlite3_handle = NULL;
}

void TestParse::cleanup()
{
	clear_dive_file_data();

	// Some test use sqlite3, ensure db is closed
	sqlite3_close(_sqlite3_handle);
}

char *intdup(int index)
{
	char tmpbuf[21];

	snprintf(tmpbuf, sizeof(tmpbuf) - 2, "%d", index);
	tmpbuf[20] = 0;
	return strdup(tmpbuf);
}

int TestParse::parseCSV(int units, std::string file)
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
	params[pnr++] = intdup(units);
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

	return parse_manual_file(file.c_str(), params, pnr - 1);
}

int TestParse::parseDivingLog()
{
	// Parsing of DivingLog import from SQLite database
	struct dive_site *ds = alloc_or_get_dive_site(0xdeadbeef);
	ds->name = copy_string("Suomi -  - Hälvälä");

	int ret = sqlite3_open(SUBSURFACE_TEST_DATA "/dives/TestDivingLog4.1.1.sql", &_sqlite3_handle);
	if ( ret == 0 )
		ret = parse_divinglog_buffer(_sqlite3_handle, 0, 0, 0, &dive_table);
	else
		fprintf(stderr, "Can't open sqlite3 db: " SUBSURFACE_TEST_DATA "/dives/TestDivingLog4.1.1.sql");

	return ret;
}

int TestParse::parseV2NoQuestion()
{
	// parsing of a V2 file should work
	return parse_file(SUBSURFACE_TEST_DATA "/dives/test40.xml");
}

int TestParse::parseV3()
{
	// parsing of a V3 files should succeed
	return parse_file(SUBSURFACE_TEST_DATA "/dives/test42.xml");
}

void TestParse::testParse()
{
	QCOMPARE(parseCSV(0, SUBSURFACE_TEST_DATA "/dives/test41.csv"), 0);
	fprintf(stderr, "number of dives %d \n", dive_table.nr);
	
	QCOMPARE(parseDivingLog(), 0);
	fprintf(stderr, "number of dives %d \n", dive_table.nr);

	QCOMPARE(parseV2NoQuestion(), 0);
	fprintf(stderr, "number of dives %d \n", dive_table.nr);

	QCOMPARE(parseV3(), 0);
	fprintf(stderr, "number of dives %d \n", dive_table.nr);

	QCOMPARE(save_dives("./testout.ssrf"), 0);
	FILE_COMPARE("./testout.ssrf",
		SUBSURFACE_TEST_DATA "/dives/test40-42.xml");
}

void TestParse::testParseDM4()
{
	QCOMPARE(sqlite3_open(SUBSURFACE_TEST_DATA "/dives/TestDiveDM4.db", &_sqlite3_handle), 0);
	QCOMPARE(parse_dm4_buffer(_sqlite3_handle, 0, 0, 0, &dive_table), 0);
	
	QCOMPARE(save_dives("./testdm4out.ssrf"), 0);
	FILE_COMPARE("./testdm4out.ssrf",
		SUBSURFACE_TEST_DATA "/dives/TestDiveDM4.xml");
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

	QCOMPARE(parse_csv_file(SUBSURFACE_TEST_DATA "/dives/TestDiveSeabearHUDC.csv",
				params, pnr - 1, "csv"), 0);

	QCOMPARE(dive_table.nr, 1);

	/*
	 * CSV import uses time and date stamps relative to current
	 * time, thus we need to use a static (random) timestamp
	 */
	if (dive_table.nr > 0) {
		struct dive *dive = dive_table.dives[dive_table.nr - 1];
		dive->when = 1255152761;
		dive->dc.when = 1255152761;
	}

	QCOMPARE(save_dives("./testhudcout.ssrf"), 0);
	FILE_COMPARE("./testhudcout.ssrf",
		SUBSURFACE_TEST_DATA "/dives/TestDiveSeabearHUDC.xml");
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

	dir = QString::fromLatin1(SUBSURFACE_TEST_DATA "/dives");
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
		QString file = QString::fromLatin1(SUBSURFACE_TEST_DATA "/dives/").append(files.at(i));
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

		char *params[42];
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
		params[pnr++] = strdup("setpointField");
		params[pnr++] = intdup(-1);
		params[pnr++] = strdup("numberField");
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

		QCOMPARE(dive_table.nr, i + 1);

		/*
		 * Set artificial but static dive times so the result
		 * can be compared to saved one.
		 */

		if (dive_table.nr > 0) {
			dive = dive_table.dives[dive_table.nr - 1];
			dive->when = 1255152761 + 7200 * i;
			dive->dc.when = 1255152761 + 7200 * i;
		}
	}

	fprintf(stderr, "number of dives %d \n", dive_table.nr);
	QCOMPARE(save_dives("./testsbnewout.ssrf"), 0);

	// Currently the CSV parse fails
	FILE_COMPARE("./testsbnewout.ssrf",
		SUBSURFACE_TEST_DATA "/dives/TestDiveSeabearNewFormat.xml");
}

void TestParse::testParseDLD()
{
	struct memblock mem;
	QString filename = SUBSURFACE_TEST_DATA "/dives/TestDiveDivelogsDE.DLD";

	QVERIFY(readfile(filename.toLatin1().data(), &mem) > 0);
	QVERIFY(try_to_open_zip(filename.toLatin1().data()) > 0);

	fprintf(stderr, "number of dives from DLD: %d \n", dive_table.nr);

	// Compare output
	/*
	 * DC is not cleared from previous tests with the
	 * clear_dive_file_data(), so we do have an additional DC nick
	 * name field on the log.
	 */
	QCOMPARE(save_dives("./testdldout.ssrf"), 0);
	FILE_COMPARE("./testdldout.ssrf",
		SUBSURFACE_TEST_DATA "/dives/TestDiveDivelogsDE.xml")
}

void TestParse::testParseMerge()
{
	/*
	 * check that we correctly merge mixed cylinder dives
	 */
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/ostc.xml"), 0);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/vyper.xml"), 0);
	QCOMPARE(save_dives("./testmerge.ssrf"), 0);
	FILE_COMPARE("./testmerge.ssrf",
		SUBSURFACE_TEST_DATA "/dives/mergedVyperOstc.xml");
}

QTEST_GUILESS_MAIN(TestParse)
