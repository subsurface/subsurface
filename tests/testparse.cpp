// SPDX-License-Identifier: GPL-2.0
#include "testparse.h"
#include "core/device.h"
#include "core/dive.h"
#include "core/divelog.h"
#include "core/divesite.h"
#include "core/errorhelper.h"
#include "core/file.h"
#include "core/import-csv.h"
#include "core/parse.h"
#include "core/qthelper.h"
#include "core/subsurface-string.h"
#include "core/trip.h"
#include "core/xmlparams.h"
#include <QTextStream>

/* We have to use a macro since QCOMPARE
 * can only be called from a test method
 * invoked by the QTest framework
 */
#define FILE_COMPARE(actual, expected)                    \
	QFile org(expected);                              \
	org.open(QFile::ReadOnly);                        \
	QFile out(actual);                                \
	out.open(QFile::ReadOnly);                        \
	QTextStream orgS(&org);                           \
	QTextStream outS(&out);                           \
	QStringList readin = orgS.readAll().split("\n");  \
	QStringList written = outS.readAll().split("\n"); \
	while (readin.size() && written.size()) {         \
		QCOMPARE(written.takeFirst().trimmed(),   \
			 readin.takeFirst().trimmed());   \
	}

void TestParse::initTestCase()
{
	TestBase::initTestCase();

	prefs = default_prefs;
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

int TestParse::parseCSV(int units, std::string file)
{
	// some basic file parsing tests
	//
	// CSV import should work
	verbose = 1;
	xml_params params;

	xml_params_add_int(&params, "numberField", 0);
	xml_params_add_int(&params, "dateField", 1);
	xml_params_add_int(&params, "timeField", 2);
	xml_params_add_int(&params, "durationField", 3);
	xml_params_add_int(&params, "locationField", -1);
	xml_params_add_int(&params, "gpsField", -1);
	xml_params_add_int(&params, "maxDepthField", 4);
	xml_params_add_int(&params, "meanDepthField", 5);
	xml_params_add_int(&params, "divemasterField", -1);
	xml_params_add_int(&params, "buddyField", 6);
	xml_params_add_int(&params, "suitField", 7);
	xml_params_add_int(&params, "notesField", -1);
	xml_params_add_int(&params, "weightField", -1);
	xml_params_add_int(&params, "tagsField", -1);
	xml_params_add_int(&params, "separatorIndex", 0);
	xml_params_add_int(&params, "units", units);
	xml_params_add_int(&params, "datefmt", 1);
	xml_params_add_int(&params, "durationfmt", 2);
	xml_params_add_int(&params, "cylindersizeField", -1);
	xml_params_add_int(&params, "startpressureField", -1);
	xml_params_add_int(&params, "endpressureField", -1);
	xml_params_add_int(&params, "o2Field", -1);
	xml_params_add_int(&params, "heField", -1);
	xml_params_add_int(&params, "airtempField", -1);
	xml_params_add_int(&params, "watertempField", -1);

	return parse_manual_file(file.c_str(), &params, &divelog);
}

int TestParse::parseDivingLog()
{
	// Parsing of DivingLog import from SQLite database
	struct dive_site *ds = divelog.sites.alloc_or_get(0xdeadbeef);
	ds->name = "Suomi -  - Hälvälä";

	int ret = sqlite3_open(SUBSURFACE_TEST_DATA "/dives/TestDivingLog4.1.1.sql", &_sqlite3_handle);
	if (ret == 0)
		ret = parse_divinglog_buffer(_sqlite3_handle, 0, 0, 0, &divelog);
	else
		fprintf(stderr, "Can't open sqlite3 db: " SUBSURFACE_TEST_DATA "/dives/TestDivingLog4.1.1.sql");

	return ret;
}

int TestParse::parseV2NoQuestion()
{
	// parsing of a V2 file should work
	return parse_file(SUBSURFACE_TEST_DATA "/dives/test40.xml", &divelog);
}

int TestParse::parseV3()
{
	// parsing of a V3 files should succeed
	return parse_file(SUBSURFACE_TEST_DATA "/dives/test42.xml", &divelog);
}

void TestParse::testParse()
{
	// On some platforms (Windows) size_t has a different format string.
	// Let's just cast to int.
	QCOMPARE(parseCSV(0, SUBSURFACE_TEST_DATA "/dives/test41.csv"), 0);
	fprintf(stderr, "number of dives %d \n", static_cast<int>(divelog.dives.size()));

	QCOMPARE(parseDivingLog(), 0);
	fprintf(stderr, "number of dives %d \n", static_cast<int>(divelog.dives.size()));

	QCOMPARE(parseV2NoQuestion(), 0);
	fprintf(stderr, "number of dives %d \n", static_cast<int>(divelog.dives.size()));

	QCOMPARE(parseV3(), 0);
	fprintf(stderr, "number of dives %d \n", static_cast<int>(divelog.dives.size()));

	QCOMPARE(save_dives("./testout.ssrf"), 0);
	FILE_COMPARE("./testout.ssrf",
		     SUBSURFACE_TEST_DATA "/dives/test40-42.xml");
}

void TestParse::testParseTankSensors()
{
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test-tank-sensor-mapping.xml", &divelog), 0);
	fprintf(stderr, "number of dives %d \n", static_cast<int>(divelog.dives.size()));

	QCOMPARE(save_dives("./testouttanksensors.ssrf"), 0);
	FILE_COMPARE("./testouttanksensors.ssrf",
		     SUBSURFACE_TEST_DATA "/dives/test-tank-sensors.xml");
}

void TestParse::testParseDM4()
{
	QCOMPARE(sqlite3_open(SUBSURFACE_TEST_DATA "/dives/TestDiveDM4.db", &_sqlite3_handle), 0);
	QCOMPARE(parse_dm4_buffer(_sqlite3_handle, 0, 0, 0, &divelog), 0);

	QCOMPARE(save_dives("./testdm4out.ssrf"), 0);
	FILE_COMPARE("./testdm4out.ssrf",
		     SUBSURFACE_TEST_DATA "/dives/TestDiveDM4.xml");
}

void TestParse::testParseDM5()
{
	QCOMPARE(sqlite3_open(SUBSURFACE_TEST_DATA "/dives/TestDiveDM5.db", &_sqlite3_handle), 0);
	QCOMPARE(parse_dm5_buffer(_sqlite3_handle, 0, 0, 0, &divelog), 0);

	QCOMPARE(save_dives("./testdm5out.ssrf"), 0);
	FILE_COMPARE("./testdm5out.ssrf",
		     SUBSURFACE_TEST_DATA "/dives/TestDiveDM5.xml");
}

void TestParse::testParseHUDC()
{
	xml_params params;

	xml_params_add_int(&params, "timeField", 0);
	xml_params_add_int(&params, "depthField", 1);
	xml_params_add_int(&params, "tempField", 5);
	xml_params_add_int(&params, "po2Field", -1);
	xml_params_add_int(&params, "o2sensor1Field", -1);
	xml_params_add_int(&params, "o2sensor2Field", -1);
	xml_params_add_int(&params, "o2sensor3Field", -1);
	xml_params_add_int(&params, "cnsField", -1);
	xml_params_add_int(&params, "ndlField", 2);
	xml_params_add_int(&params, "ttsField", -1);
	xml_params_add_int(&params, "stopdepthField", -1);
	xml_params_add_int(&params, "pressureField", -1);
	xml_params_add_int(&params, "setpointField", -1);
	xml_params_add_int(&params, "separatorIndex", 2);
	xml_params_add_int(&params, "units", 0);
	xml_params_add(&params, "hw", "\"DC text\"");

	QCOMPARE(parse_csv_file(SUBSURFACE_TEST_DATA "/dives/TestDiveSeabearHUDC.csv",
				&params, "csv", &divelog),
		 0);

	QCOMPARE(divelog.dives.size(), 1);

	/*
	 * CSV import uses time and date stamps relative to current
	 * time, thus we need to use a static (random) timestamp
	 */
	if (!divelog.dives.empty()) {
		struct dive &dive = *divelog.dives.back();
		dive.when = 1255152761;
		dive.dcs[0].when = 1255152761;
	}

	QCOMPARE(save_dives("./testhudcout.ssrf"), 0);
	FILE_COMPARE("./testhudcout.ssrf",
		     SUBSURFACE_TEST_DATA "/dives/TestDiveSeabearHUDC.xml");
}

void TestParse::testParseNewFormat()
{
	QDir dir;
	QStringList filter;
	QStringList files;

	/*
	 * Set the directory location and file filter for H3 CSV files.
	 */

	dir.setPath(QString::fromLatin1(SUBSURFACE_TEST_DATA "/dives"));
	filter << "TestDiveSeabearH3*.csv";
	filter << "TestDiveSeabearT1*.csv";
	files = dir.entryList(filter, QDir::Files);

	/*
	 * Parse all files found matching the filter.
	 */

	for (int i = 0; i < files.size(); ++i) {

		QCOMPARE(parse_seabear_log(QString::fromLatin1(SUBSURFACE_TEST_DATA
							       "/dives/")
						   .append(files.at(i))
						   .toLatin1()
						   .data(),
					   &divelog),
			 0);
		QCOMPARE(divelog.dives.size(), i + 1);
	}

	fprintf(stderr, "number of dives %d \n", static_cast<int>(divelog.dives.size()));
	QCOMPARE(save_dives("./testsbnewout.ssrf"), 0);

	// Currently the CSV parse fails
	FILE_COMPARE("./testsbnewout.ssrf",
		     SUBSURFACE_TEST_DATA "/dives/TestDiveSeabearNewFormat.xml");
}

void TestParse::testParseDLD()
{
	QString filename = SUBSURFACE_TEST_DATA "/dives/TestDiveDivelogsDE.DLD";

	auto [mem, err] = readfile(filename.toLatin1().data());
	QVERIFY(err > 0);
	QVERIFY(try_to_open_zip(filename.toLatin1().data(), &divelog) > 0);

	fprintf(stderr, "number of dives from DLD: %d \n", static_cast<int>(divelog.dives.size()));

	// Compare output
	QCOMPARE(save_dives("./testdldout.ssrf"), 0);
	FILE_COMPARE("./testdldout.ssrf",
		     SUBSURFACE_TEST_DATA "/dives/TestDiveDivelogsDE.xml")
}

void TestParse::testParseMerge()
{
	/*
	 * check that we correctly merge mixed cylinder dives
	 */
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/ostc.xml", &divelog), 0);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/vyper.xml", &divelog), 0);

	QCOMPARE(save_dives("./testmerge.ssrf"), 0);
	FILE_COMPARE("./testmerge.ssrf",
		     SUBSURFACE_TEST_DATA "/dives/mergedVyperOstc.xml");
}

void TestParse::testParseMergeTankSensors()
{
	/*
	 * check that we correctly merge mixed cylinder dives with tank sensors
	 */
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test-tank-sensor-mapping.xml", &divelog), 0);

	struct divelog divelogToMerge;
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test-tank-sensor-mapping-merge.xml", &divelogToMerge), 0);
	divelog.add_imported_dives(divelogToMerge, import_flags::merge_all_trips);

	QCOMPARE(save_dives("./testmergetanksensors.ssrf"), 0);
	FILE_COMPARE("./testmergetanksensors.ssrf",
		     SUBSURFACE_TEST_DATA "/dives/merged-tank-sensors.xml");
}

int TestParse::parseCSVmanual(int units, std::string file)
{
	verbose = 1;
	xml_params params;

	// Numbers are column numbers
	xml_params_add_int(&params, "numberField", 0);
	xml_params_add_int(&params, "dateField", 1);
	xml_params_add_int(&params, "timeField", 2);
	xml_params_add_int(&params, "durationField", 3);
	// 4 Will be SAC, once we add support for reading it
	xml_params_add_int(&params, "maxDepthField", 5);
	xml_params_add_int(&params, "meanDepthField", 6);
	xml_params_add_int(&params, "modeField", 7);
	xml_params_add_int(&params, "airtempField", 8);
	xml_params_add_int(&params, "watertempField", 9);
	xml_params_add_int(&params, "cylindersizeField", 10);
	xml_params_add_int(&params, "startpressureField", 11);
	xml_params_add_int(&params, "endpressureField", 12);
	xml_params_add_int(&params, "o2Field", 13);
	xml_params_add_int(&params, "heField", 14);
	xml_params_add_int(&params, "locationField", 15);
	xml_params_add_int(&params, "gpsField", 16);
	xml_params_add_int(&params, "divemasterField", 17);
	xml_params_add_int(&params, "buddyField", 18);
	xml_params_add_int(&params, "suitField", 19);
	xml_params_add_int(&params, "notesField", 22);
	xml_params_add_int(&params, "weightField", 23);
	xml_params_add_int(&params, "tagsField", 24);
	// Numbers are indices of possible options
	xml_params_add_int(&params, "separatorIndex", 1);
	xml_params_add_int(&params, "datefmt", 2);
	xml_params_add_int(&params, "durationfmt", 2);
	xml_params_add_int(&params, "units", units);
	return parse_manual_file(file.c_str(), &params, &divelog);
}

void TestParse::exportCSVDiveDetails()
{
	int saved_sac = 0;

	parse_file(SUBSURFACE_TEST_DATA "/dives/test25.xml", &divelog);

	export_dives_xslt("testcsvexportmanual.csv", 0, 0, "xml2manualcsv.xslt", false);
	export_dives_xslt("testcsvexportmanualimperial.csv", 0, 1, "xml2manualcsv.xslt", false);

	if (!divelog.dives.empty())
		saved_sac = divelog.dives.back()->sac;
	clear_dive_file_data();

	parseCSVmanual(1, "testcsvexportmanualimperial.csv");

	// We do not currently support reading SAC, thus faking it
	if (!divelog.dives.empty())
		divelog.dives.back()->sac = saved_sac;

	export_dives_xslt("testcsvexportmanual2.csv", 0, 0, "xml2manualcsv.xslt", false);
	FILE_COMPARE("testcsvexportmanual2.csv",
		     "testcsvexportmanual.csv");
}

void TestParse::exportSubsurfaceCSV()
{
	int saved_sac = 0;
	xml_params params;

	/* Test SubsurfaceCSV with multiple cylinders */
	parse_file(SUBSURFACE_TEST_DATA "/dives/test40.xml", &divelog);

	export_dives_xslt("testcsvexportmanual-cyl.csv", 0, 0, "xml2manualcsv.xslt", false);
	export_dives_xslt("testcsvexportmanualimperial-cyl.csv", 0, 1, "xml2manualcsv.xslt", false);

	if (!divelog.dives.empty())
		saved_sac = divelog.dives.back()->sac;

	clear_dive_file_data();

	xml_params_add_int(&params, "separatorIndex", 1);
	xml_params_add_int(&params, "units", 1);
	parse_csv_file("testcsvexportmanualimperial-cyl.csv", &params, "SubsurfaceCSV", &divelog);

	// We do not currently support reading SAC, thus faking it
	if (!divelog.dives.empty())
		divelog.dives.back()->sac = saved_sac;

	export_dives_xslt("testcsvexportmanual2-cyl.csv", 0, 0, "xml2manualcsv.xslt", false);
	FILE_COMPARE("testcsvexportmanual2-cyl.csv",
		     "testcsvexportmanual-cyl.csv");
}

int TestParse::parseCSVprofile(int units, std::string file)
{
	verbose = 1;
	xml_params params;

	// Numbers are column numbers
	xml_params_add_int(&params, "numberField", 0);
	xml_params_add_int(&params, "dateField", 1);
	xml_params_add_int(&params, "starttimeField", 2);
	xml_params_add_int(&params, "timeField", 3);
	xml_params_add_int(&params, "depthField", 4);
	xml_params_add_int(&params, "tempField", 5);
	xml_params_add_int(&params, "pressureField", 6);
	// Numbers are indices of possible options
	xml_params_add_int(&params, "datefmt", 2);
	xml_params_add_int(&params, "units", units);

	return parse_csv_file(file.c_str(), &params, "csv", &divelog);
}

void TestParse::exportCSVDiveProfile()
{
	parse_file(SUBSURFACE_TEST_DATA "/dives/test40.xml", &divelog);

	export_dives_xslt("testcsvexportprofile.csv", 0, 0, "xml2csv.xslt", false);
	export_dives_xslt("testcsvexportprofileimperial.csv", 0, 1, "xml2csv.xslt", false);

	clear_dive_file_data();

	parseCSVprofile(1, "testcsvexportprofileimperial.csv");

	export_dives_xslt("testcsvexportprofile2.csv", 0, 0, "xml2csv.xslt", false);
	FILE_COMPARE("testcsvexportprofile2.csv",
		     "testcsvexportprofile.csv");
}

void TestParse::exportUDDF()
{
	parse_file(SUBSURFACE_TEST_DATA "/dives/test40.xml", &divelog);

	export_dives_xslt("testuddfexport.uddf", 0, 1, "uddf-export.xslt", false);

	clear_dive_file_data();

	parse_file("testuddfexport.uddf", &divelog);

	export_dives_xslt("testuddfexport2.uddf", 0, 1, "uddf-export.xslt", false);
	FILE_COMPARE("testuddfexport.uddf",
		     "testuddfexport2.uddf");
}

void TestParse::parseDL7()
{
	xml_params params;

	xml_params_add_int(&params, "dateField", -1);
	xml_params_add_int(&params, "datefmt", 0);
	xml_params_add_int(&params, "starttimeField", -1);
	xml_params_add_int(&params, "numberField", -1);
	xml_params_add_int(&params, "timeField", 1);
	xml_params_add_int(&params, "depthField", 2);
	xml_params_add_int(&params, "tempField", -1);
	xml_params_add_int(&params, "po2Field", -1);
	xml_params_add_int(&params, "o2sensor1Field", -1);
	xml_params_add_int(&params, "o2sensor2Field", -1);
	xml_params_add_int(&params, "o2sensor3Field", -1);
	xml_params_add_int(&params, "cnsField", -1);
	xml_params_add_int(&params, "ndlField", -1);
	xml_params_add_int(&params, "ttsField", -1);
	xml_params_add_int(&params, "stopdepthField", -1);
	xml_params_add_int(&params, "pressureField", -1);
	xml_params_add_int(&params, "setpointField", -1);
	xml_params_add_int(&params, "separatorIndex", 3);
	xml_params_add_int(&params, "units", 0);
	xml_params_add(&params, "hw", "DL7");

	clear_dive_file_data();
	QCOMPARE(parse_csv_file(SUBSURFACE_TEST_DATA "/dives/DL7.zxu",
				&params, "DL7", &divelog),
		 0);

	QCOMPARE(divelog.dives.size(), 3);
	QCOMPARE(divelog.dives[0]->number, 1);
	QCOMPARE(divelog.dives[1]->number, 2);
	QCOMPARE(divelog.dives[2]->number, 3);

	QCOMPARE(save_dives("./testdl7out.ssrf"), 0);
	FILE_COMPARE("./testdl7out.ssrf",
		     SUBSURFACE_TEST_DATA "/dives/DL7.xml");
}

void TestParse::importApdInspirationUddf()
{
	parse_file(SUBSURFACE_TEST_DATA "/dives/test-apd-inspiration.uddf", &divelog);

	QCOMPARE(save_dives("./testapdinspiration.xml"), 0);
	FILE_COMPARE("./testapdinspiration.xml",
		     SUBSURFACE_TEST_DATA "/dives/test-apd-inspiration-reference.xml");
}


QTEST_GUILESS_MAIN(TestParse)
