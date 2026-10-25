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
#include "core/pref.h"
#include "core/qthelper.h"
#include "core/subsurface-string.h"
#include "core/trip.h"
#include "core/xmlparams.h"
#include <libxml/parser.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <QTextStream>
#include <QXmlStreamReader>

#include <algorithm>
#include <cstdint>
#include <vector>

/* We have to use a macro since QCOMPARE
 * can only be called from a test method
 * invoked by the QTest framework
 */
#define FILE_COMPARE(actual, expected) \
	do { \
		QFile org(expected); \
		org.open(QFile::ReadOnly); \
		QFile out(actual); \
		out.open(QFile::ReadOnly); \
		QTextStream orgS(&org); \
		QTextStream outS(&out); \
		QStringList readin = orgS.readAll().split("\n"); \
		QStringList written = outS.readAll().split("\n"); \
		while (readin.size() && written.size()) { \
			QCOMPARE(written.takeFirst().trimmed(), \
				readin.takeFirst().trimmed()); \
		} \
	} while (false)

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

int TestParse::parseCSV(int units, std::string file, int separatorIndex)
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
	xml_params_add_int(&params, "notesField", 8);
	xml_params_add_int(&params, "weightField", -1);
	xml_params_add_int(&params, "tagsField", -1);
	xml_params_add_int(&params, "separatorIndex", separatorIndex);
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

void TestParse::exportSelectedTrip()
{
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test-selected-trip-export.xml", &divelog), 0);
	QCOMPARE(divelog.dives.size(), size_t(5));
	QCOMPARE(divelog.trips.size(), size_t(2));
	divelog.dives[0]->selected = true;
	divelog.dives[2]->selected = true;
	divelog.dives[4]->selected = true;

	QCOMPARE(save_dives_logic("./testselectedtrip.ssrf", true, false), 0);
	QFile output("./testselectedtrip.ssrf");
	QVERIFY(output.open(QFile::ReadOnly));
	const QString xml = QString::fromUtf8(output.readAll());
	QCOMPARE(xml.count("<trip"), 1);
	QVERIFY(xml.contains("location='Selected Trip'"));
	QVERIFY(xml.contains("<notes>Trip notes</notes>"));
	QVERIFY(!xml.contains("Unselected Trip"));
	QVERIFY(xml.contains("number='101'"));
	QVERIFY(xml.contains("number='103'"));
	QVERIFY(xml.contains("number='105'"));
	QVERIFY(xml.indexOf("<trip") < xml.indexOf("number='101'"));
	QVERIFY(xml.indexOf("number='101'") < xml.indexOf("number='103'"));
	QVERIFY(xml.indexOf("number='103'") < xml.indexOf("</trip>"));
	QVERIFY(xml.indexOf("</trip>") < xml.indexOf("number='105'"));
	QVERIFY(!xml.contains("number='102'"));
	QVERIFY(!xml.contains("number='104'"));
	output.close();

	QCOMPARE(save_dives_logic("./testselectedtrip-anonymized.ssrf", true, true), 0);
	QFile anonymizedOutput("./testselectedtrip-anonymized.ssrf");
	QVERIFY(anonymizedOutput.open(QFile::ReadOnly));
	const QString anonymizedXml = QString::fromUtf8(anonymizedOutput.readAll());
	QVERIFY(anonymizedXml.contains("location='Xxxxxxxx Xxxx'"));
	QVERIFY(anonymizedXml.contains("<notes>Xxxx xxxxx</notes>"));
	QVERIFY(!anonymizedXml.contains("Selected Trip"));
	QVERIFY(!anonymizedXml.contains("Trip notes"));
}

void TestParse::testParseTsv()
{
	// On some platforms (Windows) size_t has a different format string.
	// Let's just cast to int.
	QCOMPARE(parseCSV(0, SUBSURFACE_TEST_DATA "/dives/test41.tsv", 0), 0);
	fprintf(stderr, "number of dives %d \n", static_cast<int>(divelog.dives.size()));

	QCOMPARE(save_dives("./testouttsv.ssrf"), 0);
	FILE_COMPARE("./testouttsv.ssrf",
		     SUBSURFACE_TEST_DATA "/dives/test-tsv.xml");
}

void TestParse::testParseCsv()
{
	// On some platforms (Windows) size_t has a different format string.
	// Let's just cast to int.
	QCOMPARE(parseCSV(0, SUBSURFACE_TEST_DATA "/dives/test41.csv", 1), 0);
	fprintf(stderr, "number of dives %d \n", static_cast<int>(divelog.dives.size()));

	QCOMPARE(save_dives("./testoutcsv.ssrf"), 0);
	FILE_COMPARE("./testoutcsv.ssrf",
		     SUBSURFACE_TEST_DATA "/dives/test-csv.xml");
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

// AI-generated (Claude)
// A Shearwater Cloud database whose dive_logs and dive_log_records tables are
// empty, so the dives can only come from the raw logs in log_data
void TestParse::testParseShearwaterCloud()
{
	QCOMPARE(sqlite3_open(SUBSURFACE_TEST_DATA "/dives/TestDiveShearwaterCloud.db", &_sqlite3_handle), 0);
	QCOMPARE(parse_shearwater_cloud_buffer(_sqlite3_handle, 0, 0, 0, &divelog), 0);
	QCOMPARE(divelog.dives.size(), 2);

	QCOMPARE(save_dives("./testshearwatercloudout.ssrf"), 0);
	FILE_COMPARE("./testshearwatercloudout.ssrf",
		     SUBSURFACE_TEST_DATA "/dives/TestDiveShearwaterCloud.xml");
}

// AI-generated (Claude)
// The Perdix 2 dive from TestDiveShearwaterCloud.db, changed so that the
// header parser first creates a dive site from a GPS fix and then fails:
// opening record 4 byte 28 (air integration mode) is AI_ON_GPS, opening
// record 9 holds a 3D fix at 9.07 N 123.27 E, and opening record 4 byte 1
// (dive mode) is 0x7F, which no Shearwater computer uses. The dive must be
// dropped without leaving its dive site behind.
void TestParse::testParseShearwaterCloudGpsError()
{
	QCOMPARE(sqlite3_open(SUBSURFACE_TEST_DATA "/dives/TestDiveShearwaterCloudGpsError.db", &_sqlite3_handle), 0);
	// The header error is reported by design, so don't fail on it
	set_error_cb([](std::string) {});
	int ret = parse_shearwater_cloud_buffer(_sqlite3_handle, 0, 0, 0, &divelog);
	set_error_cb(&TestBase::failOnError);
	QCOMPARE(ret, 0);
	QCOMPARE(divelog.dives.size(), 0);
	QCOMPARE(divelog.sites.size(), 0);
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
		     SUBSURFACE_TEST_DATA "/dives/TestDiveDivelogsDE.xml");
}

void TestParse::testImportOSTCTools()
{
#if defined(SUBSURFACE_MOBILE)
	QSKIP("Not testing OSTCTools import on SUBSURFACE_MOBILE");
#else
	const char *samples[] = {
		"ostc_00087_04-05-2014_043m_032min.dive",
		"ostc_00173_17-08-2013_027m_043min.dive",
	};
	const QString expected_model =
		QStringLiteral("Heinrichs Weikamp OSTC 2N (Imported from OSTCTools)");

	for (unsigned int i = 0; i < sizeof(samples) / sizeof(samples[0]); ++i) {
		QString filename = QString::fromLatin1(SUBSURFACE_TEST_DATA "/dives/") + samples[i];
		auto [buffer, error] = readfile(filename.toLatin1().data());
		QVERIFY(error > 0);
		QVERIFY(ostctools_import(buffer, &divelog) > 0);
		QCOMPARE(divelog.dives.size(), i + 1);
		QCOMPARE(QString::fromStdString(divelog.dives[i]->dcs[0].model), expected_model);
	}

	// AI-generated (Claude): Minimal valid hwOS v0x23 records test the imported display labels.
	struct hwos_sample {
		unsigned int serial;
		const char *model;
	};
	const hwos_sample hwos_samples[] = {
		{10000, "OSTC 3"},
		{10001, "OSTC Sport"},
	};
	for (unsigned int i = 0; i < sizeof(hwos_samples) / sizeof(hwos_samples[0]); ++i) {
		std::string buffer(456 + 256 + 2, '\0');
		buffer[265] = hwos_samples[i].serial & 0xff;
		buffer[266] = hwos_samples[i].serial >> 8;
		buffer[456 + 8] = 0x23;
		buffer[456 + 12] = 24;
		buffer[456 + 13] = 1;
		buffer[456 + 14] = 1;
		for (unsigned int gas = 0; gas < 5; ++gas) {
			buffer[456 + 28 + 4 * gas] = 21;
			buffer[456 + 31 + 4 * gas] = 1;
		}
		buffer[456 + 256] = 0xFD;
		buffer[456 + 257] = 0xFD;
		QVERIFY(ostctools_import(buffer, &divelog) > 0);
		QCOMPARE(divelog.dives.size(), i + sizeof(samples) / sizeof(samples[0]) + 1);
		QCOMPARE(QString::fromStdString(divelog.dives.back()->dcs[0].model),
			 QStringLiteral("Heinrichs Weikamp %1 (Imported from OSTCTools)").arg(hwos_samples[i].model));
	}
#endif
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

	export_dives_xslt("testcsvexportmanual.csv", 0, 0, "xml2summarycsv.xslt", false);
	export_dives_xslt("testcsvexportmanualimperial.csv", 0, 1, "xml2summarycsv.xslt", false);

	if (!divelog.dives.empty())
		saved_sac = divelog.dives.back()->sac;
	clear_dive_file_data();

	parseCSVmanual(1, "testcsvexportmanualimperial.csv");

	// We do not currently support reading SAC, thus faking it
	if (!divelog.dives.empty())
		divelog.dives.back()->sac = saved_sac;

	export_dives_xslt("testcsvexportmanual2.csv", 0, 0, "xml2summarycsv.xslt", false);
	FILE_COMPARE("testcsvexportmanual2.csv",
		     "testcsvexportmanual.csv");
}

void TestParse::exportSubsurfaceCSV()
{
	/* Test SubsurfaceCSV with multiple cylinders */
	parse_file(SUBSURFACE_TEST_DATA "/dives/test42.xml", &divelog);

	export_dives_xslt("testcsvexportmanual-cyl.csv", 0, 0, "xml2summarycsv.xslt", false);
	export_dives_xslt("testcsvexportmanualimperial-cyl.csv", 0, 1, "xml2summarycsv.xslt", false);

	int saved_sac = 0;
	if (!divelog.dives.empty())
		saved_sac = divelog.dives.back()->sac;

	clear_dive_file_data();

	xml_params params;
	xml_params_add_int(&params, "separatorIndex", 1);
	xml_params_add_int(&params, "units", 1);
	parse_csv_file("testcsvexportmanualimperial-cyl.csv", &params, "SubsurfaceCSV", &divelog);

	// We do not currently support reading SAC, thus faking it
	if (!divelog.dives.empty())
		divelog.dives.back()->sac = saved_sac;

	QCOMPARE(save_dives("./testcsvexport.ssrf"), 0);
	FILE_COMPARE("./testcsvexport.ssrf", SUBSURFACE_TEST_DATA "/dives/test42-csv.xml");

	export_dives_xslt("testcsvexportmanual2-cyl.csv", 0, 0, "xml2summarycsv.xslt", false);
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
	parse_file(SUBSURFACE_TEST_DATA "/dives/test42.xml", &divelog);

	export_dives_xslt("testcsvexportprofile.csv", 0, 0, "xml2detailscsv.xslt", false);
	export_dives_xslt("testcsvexportprofileimperial.csv", 0, 1, "xml2detailscsv.xslt", false);

	clear_dive_file_data();

	parseCSVprofile(1, "testcsvexportprofileimperial.csv");

	export_dives_xslt("testcsvexportprofile2.csv", 0, 0, "xml2detailscsv.xslt", false);
	FILE_COMPARE("testcsvexportprofile2.csv",
		     "testcsvexportprofile.csv");
}

void TestParse::exportUDDF()
{
	parse_file(SUBSURFACE_TEST_DATA "/dives/test42.xml", &divelog);

	export_dives_xslt("testuddfexport.uddf", 0, 1, "uddf-export.xslt", false);
	FILE_COMPARE("testuddfexport.uddf", SUBSURFACE_TEST_DATA "/dives/test42.uddf");
}

// AI-generated (Claude): Regression coverage for interleaved UDDF events and samples.
void TestParse::exportUDDFEvents()
{
	xmlDoc *input = xmlReadFile(SUBSURFACE_TEST_DATA "/dives/test-uddf-export-events.xml", NULL, XML_PARSE_HUGE);
	QVERIFY(input);
	xsltStylesheetPtr stylesheet = get_stylesheet("uddf-export.xslt");
	QVERIFY(stylesheet);
	const char *params[] = { "units", "'1'", NULL };
	xmlDoc *output = xsltApplyStylesheet(stylesheet, input, params);
	QVERIFY(output);
	QVERIFY(xsltSaveResultToFilename("testuddfevents.uddf", output, stylesheet, 0) > 0);
	xmlFreeDoc(output);
	xsltFreeStylesheet(stylesheet);
	xmlFreeDoc(input);
	QFile expected(SUBSURFACE_TEST_DATA "/dives/test-uddf-export-events.uddf");
	QVERIFY(expected.open(QFile::ReadOnly));
	QFile actual("testuddfevents.uddf");
	QVERIFY(actual.open(QFile::ReadOnly));
	QCOMPARE(actual.readAll(), expected.readAll());
}

static std::vector<const dive_site *> allDiveSites()
{
	std::vector<const dive_site *> sites;
	sites.reserve(divelog.sites.size());
	for (const auto &site: divelog.sites)
		sites.push_back(site.get());
	return sites;
}

void TestParse::exportDiveSitesXML()
{
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test42.xml", &divelog), 0);
	dive_site *site = divelog.sites.get_by_uuid(0xec2bbc32);
	QVERIFY(site);
	site->name = "Lake & Ice";
	site->description = "Alpine <lake>";
	site->notes = "Bring a drysuit";
	taxonomy_set_category(site->taxonomy, TC_COUNTRY, "New Zealand", GEOMANUAL);

	auto result = export_dive_sites_xslt("testdivesites.xml", allDiveSites(), "divesites-export.xslt", false);
	QCOMPARE(result.first, 0);
	QVERIFY2(result.second.empty(), result.second.c_str());

	clear_dive_file_data();
	QCOMPARE(parse_file("testdivesites.xml", &divelog), 0);
	site = divelog.sites.get_by_uuid(0xec2bbc32);
	QVERIFY(site);
	QCOMPARE(site->name, std::string("Lake & Ice"));
	QCOMPARE(site->description, std::string("Alpine <lake>"));
	QCOMPARE(site->notes, std::string("Bring a drysuit"));
	QCOMPARE(site->location.lat.udeg, -43342295);
	QCOMPARE(site->location.lon.udeg, 171545936);
	QCOMPARE(taxonomy_get_value(site->taxonomy, TC_COUNTRY), std::string("New Zealand"));
}

void TestParse::exportKML()
{
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test42.xml", &divelog), 0);
	dive_site *site = divelog.sites.get_by_uuid(0xec2bbc32);
	QVERIFY(site);
	site->name = "Lake & Ice";
	site->description = "Alpine <lake>";
	site->notes = "Bring a drysuit";
	divelog.sites.create("Site without coordinates");

	auto result = export_dive_sites_xslt("testdivesites.kml", allDiveSites(), "kml-export.xslt", false);
	QCOMPARE(result.first, 0);
	QVERIFY2(result.second.empty(), result.second.c_str());

	QFile file("testdivesites.kml");
	QVERIFY(file.open(QIODevice::ReadOnly));
	QXmlStreamReader xml(&file);
	int placemarks = 0;
	QString name;
	QString description;
	QString coordinates;
	while (!xml.atEnd()) {
		xml.readNext();
		if (!xml.isStartElement())
			continue;
		if (xml.name() == QStringLiteral("kml"))
			QCOMPARE(xml.namespaceUri().toString(), QStringLiteral("http://www.opengis.net/kml/2.2"));
		else if (xml.name() == QStringLiteral("Placemark"))
			placemarks++;
		else if (xml.name() == QStringLiteral("name") && placemarks > 0)
			name = xml.readElementText();
		else if (xml.name() == QStringLiteral("description"))
			description = xml.readElementText();
		else if (xml.name() == QStringLiteral("coordinates"))
			coordinates = xml.readElementText().trimmed();
	}

	QVERIFY2(!xml.hasError(), qPrintable(xml.errorString()));
	QCOMPARE(placemarks, 1);
	QCOMPARE(name, QStringLiteral("Lake & Ice"));
	QCOMPARE(description, QStringLiteral("Alpine <lake>\nBring a drysuit"));
	QCOMPARE(coordinates, QStringLiteral("171.545936,-43.342295,0"));

#if defined(Q_OS_LINUX)
	result = export_dive_sites_xslt("/dev/full", allDiveSites(), "kml-export.xslt", false);
	QVERIFY(result.first != 0);
	QVERIFY(!result.second.empty());
#endif
}

void TestParse::importUDDF()
{
	parse_file(SUBSURFACE_TEST_DATA "/dives/test42.uddf", &divelog);

	QCOMPARE(save_dives("./testuddfimport.xml"), 0);
	FILE_COMPARE("./testuddfimport.xml",
		     SUBSURFACE_TEST_DATA "/dives/test-42-uddf-import-reference.xml");
}

// Regression coverage for importing a UDDF waypoint where <alarm> precedes
// its own <divetime> (the order Subsurface's own UDDF export uses throughout):
// the alarm's event time must still be picked up via the following-sibling
// fallback, not silently dropped to 0.
void TestParse::importUDDFEvents()
{
	parse_file(SUBSURFACE_TEST_DATA "/dives/test-uddf-export-events.uddf", &divelog);

	QCOMPARE(save_dives("./testuddfeventsimport.xml"), 0);
	FILE_COMPARE("./testuddfeventsimport.xml",
		     SUBSURFACE_TEST_DATA "/dives/test-uddf-export-events-import-reference.xml");
}

void TestParse::exportDiveLogsDE()
{
	parse_file(SUBSURFACE_TEST_DATA "/dives/test40.xml", &divelog);
	export_dives_xslt("testdivelogsexport.divelogs", 0, 0, "divelogs-export.xslt", false);
	FILE_COMPARE("testdivelogsexport.divelogs", SUBSURFACE_TEST_DATA "/dives/test40.divelogs");
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

// AI-generated (Claude): Regression coverage for namespace normalisation in
// test_xslt_transforms(). test-uddf-ccr-3.2.uddf and test-uddf-ccr-3.1.uddf
// declare the UDDF default namespace on the root element (3.2 and 3.1
// respectively) and are otherwise byte-identical; a CCR dive with a mid-dive
// switch to open circuit on a bailout gas must import exactly the same way
// in both namespaces, matching what an un-namespaced UDDF file already does.
void TestParse::importUddfCcrNamespace32()
{
	parse_file(SUBSURFACE_TEST_DATA "/dives/test-uddf-ccr-3.2.uddf", &divelog);

	QCOMPARE(save_dives("./testuddfccrnamespace32.xml"), 0);
	FILE_COMPARE("./testuddfccrnamespace32.xml",
		     SUBSURFACE_TEST_DATA "/dives/test-uddf-ccr-reference.xml");
}

void TestParse::importUddfCcrNamespace31()
{
	parse_file(SUBSURFACE_TEST_DATA "/dives/test-uddf-ccr-3.1.uddf", &divelog);

	QCOMPARE(save_dives("./testuddfccrnamespace31.xml"), 0);
	FILE_COMPARE("./testuddfccrnamespace31.xml",
		     SUBSURFACE_TEST_DATA "/dives/test-uddf-ccr-reference.xml");
}

// AI-generated (Claude): Boundary coverage for the namespace shape check in
// xml_ns_has_prefix()/looks_like_uddf_version_suffix() (core/parse-xml.cpp).
// test-uddf-non-version-namespace.uddf is test-uddf-ccr-3.2.uddf with its
// namespace changed to "http://www.streit.cc/uddf/not-a-version/": it shares
// the UDDF URI string prefix but its suffix is not shaped like a version
// number, so it must not be recognised/stripped as a UDDF namespace. Since
// uddf.xslt only matches un-prefixed (or known-namespace) paths, the dive
// stays namespaced and import of this file silently finds no dive.
void TestParse::importUddfNonVersionNamespaceIsNotStripped()
{
	parse_file(SUBSURFACE_TEST_DATA "/dives/test-uddf-non-version-namespace.uddf", &divelog);

	QCOMPARE(divelog.dives.size(), 0);
}

// AI-generated (Claude): Return the "duration" attribute of the first <dive>
// in the intermediate XML that xslt/uddf.xslt produces for filename, i.e. the
// stylesheet's raw output before fixup_dc_duration() (core/divecomputer.cpp)
// runs. Applies the stylesheet via test_xslt_transforms(), the same dispatch
// parse_xml_buffer() uses before traversing the document (including any
// namespace normalisation uddf.xslt relies on), so this reflects exactly
// what a UDDF import feeds to the parser.
static QString uddfTransformedDiveDuration(const char *filename)
{
	xmlDoc *input = xmlReadFile(filename, NULL, XML_PARSE_HUGE);
	if (!input)
		return QString();
	xmlDoc *transformed = test_xslt_transforms(input, NULL);
	if (!transformed)
		return QString();

	xmlChar *duration = nullptr;
	xmlNode *root = xmlDocGetRootElement(transformed);
	for (xmlNode *n = root ? xmlFirstElementChild(root) : nullptr; n; n = xmlNextElementSibling(n)) {
		if (strcmp((const char *)n->name, "dives") != 0)
			continue;
		for (xmlNode *dive = xmlFirstElementChild(n); dive; dive = xmlNextElementSibling(dive)) {
			if (strcmp((const char *)dive->name, "dive") == 0) {
				duration = xmlGetProp(dive, (const xmlChar *)"duration");
				break;
			}
		}
		break;
	}

	QString result = duration ? QString::fromUtf8((const char *)duration) : QString();
	xmlFree(duration);
	xmlFreeDoc(transformed);
	return result;
}

// AI-generated (Claude): Regression coverage for an XPath 1.0 node-set-to-number
// pitfall in xslt/uddf.xslt's weight and duration selections. Comparing a
// node-set to a number (the surrounding <xsl:if> tests) is existential across
// every node in the set, but converting a node-set to a number (what
// format-number()/value-of do to produce the output value) uses only the
// string-value of the first node in document order. test-uddf-zero-before-
// positive.uddf puts a zero-valued alternative earlier in document order than
// a positive-valued one for both the weight union
// (informationbeforedive/equipmentused/leadquantity = 0, before
// informationafterdive/equipmentused/leadquantity = 4) and the duration union
// (dive_duration = 0, before informationafterdive/diveduration = 300): taking
// the first node unconditionally would silently import a 0.0 kg weight and a
// 0:00 min duration instead of the real 4.0 kg / 5:00 min.
void TestParse::importUddfZeroBeforePositive()
{
	parse_file(SUBSURFACE_TEST_DATA "/dives/test-uddf-zero-before-positive.uddf", &divelog);

	// This round-trip genuinely exercises the weight-union fix (nothing
	// overwrites weightsystem data after import), but NOT the
	// duration-union fix: fixup_dc_duration() (core/divecomputer.cpp)
	// unconditionally recomputes dc.duration.seconds from the dive's
	// sample depths/times on every import, discarding whatever duration
	// the stylesheet selected. This fixture's sample profile happens to
	// independently recompute to the same 300s/"5:00 min" regardless of
	// whether the duration-union fix is applied, so comparing against the
	// saved-and-reloaded dive below would pass even with that half of the
	// fix fully reverted.
	QCOMPARE(save_dives("./testuddfzerobeforepositive.xml"), 0);
	FILE_COMPARE("./testuddfzerobeforepositive.xml",
		     SUBSURFACE_TEST_DATA "/dives/test-uddf-zero-before-positive-reference.xml");

	// Assert on the stylesheet's raw, pre-fixup output instead, which is
	// the only place the duration-union fix is actually observable.
	QCOMPARE(uddfTransformedDiveDuration(SUBSURFACE_TEST_DATA "/dives/test-uddf-zero-before-positive.uddf"),
		 QString("5:00 min"));
}

void TestParse::importDlfFreedomV1()
{
#if defined(SUBSURFACE_MOBILE)
	QSKIP("Not testing DLF import on SUBSURFACE_MOBILE");
#endif
	parse_file(SUBSURFACE_TEST_DATA "/dives/Freedom_MIX_header_v1_00000115.dlf", &divelog);
	QCOMPARE(save_dives("./test_import_Freedom_MIX_header_v1_00000115.dlf.xml"), 0);
	FILE_COMPARE("./test_import_Freedom_MIX_header_v1_00000115.dlf.xml",
			SUBSURFACE_TEST_DATA "/dives/Freedom_MIX_header_v1_00000115.dlf.xml");
}

void TestParse::importDlfLibertyV1()
{
#if defined(SUBSURFACE_MOBILE)
	QSKIP("Not testing DLF import on SUBSURFACE_MOBILE");
#endif
	parse_file(SUBSURFACE_TEST_DATA "/dives/Liberty_CCR_header_v1_00000011.dlf", &divelog);
	QCOMPARE(save_dives("./test_import_Liberty_CCR_header_v1_00000011.dlf.xml"), 0);
	FILE_COMPARE("./test_import_Liberty_CCR_header_v1_00000011.dlf.xml",
			SUBSURFACE_TEST_DATA "/dives/Liberty_CCR_header_v1_00000011.dlf.xml");
}

void TestParse::importDlfFreedomMixV2()
{
#if defined(SUBSURFACE_MOBILE)
	QSKIP("Not testing DLF import on SUBSURFACE_MOBILE");
#endif
	parse_file(SUBSURFACE_TEST_DATA "/dives/Freedom_MIX_header_v2_00000003.dlf", &divelog);
	QCOMPARE(save_dives("./test_import_Freedom_MIX_header_v2_00000003.dlf.xml"), 0);
	FILE_COMPARE("./test_import_Freedom_MIX_header_v2_00000003.dlf.xml",
			SUBSURFACE_TEST_DATA "/dives/Freedom_MIX_header_v2_00000003.dlf.xml");
}

void TestParse::importDlfFreedomMix2V2()
{
#if defined(SUBSURFACE_MOBILE)
	QSKIP("Not testing DLF import on SUBSURFACE_MOBILE");
#endif
	parse_file(SUBSURFACE_TEST_DATA "/dives/Freedom_MIX2_header_v2_00000007.dlf", &divelog);
	QCOMPARE(save_dives("./test_import_Freedom_MIX2_header_v2_00000007.dlf.xml"), 0);
	FILE_COMPARE("./test_import_Freedom_MIX2_header_v2_00000007.dlf.xml",
			SUBSURFACE_TEST_DATA "/dives/Freedom_MIX2_header_v2_00000007.dlf.xml");
}

void TestParse::importDlfFreedomMix2V2FactoryTest()
{
#if defined(SUBSURFACE_MOBILE)
	QSKIP("Not testing DLF import on SUBSURFACE_MOBILE");
#endif
	parse_file(SUBSURFACE_TEST_DATA "/dives/Freedom_MIX2_header_v2_factory_test_00000001.dlf", &divelog);
	QCOMPARE(save_dives("./test_import_Freedom_MIX2_header_v2_factory_test_00000001.dlf.xml"), 0);
	FILE_COMPARE("./test_import_Freedom_MIX2_header_v2_factory_test_00000001.dlf.xml",
			SUBSURFACE_TEST_DATA "/dives/Freedom_MIX2_header_v2_factory_test_00000001.dlf.xml");
}

void TestParse::importSuuntoJsonNautic()
{
#if defined(SUBSURFACE_MOBILE)
	QSKIP("Not testing Suunto JSON import on SUBSURFACE_MOBILE");
#endif
	/* Suunto Nautic, sidemount, two cylinders air, two transmitters.
	 * Tests the Nautic/Ocean JSON variant (DiveEvents, 0-based GasNumber). */
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/suunto_nautic_sidemount.json", &divelog), 0);
	QCOMPARE(save_dives("./test_suunto_nautic_sidemount.ssrf"), 0);
	FILE_COMPARE("./test_suunto_nautic_sidemount.ssrf",
		SUBSURFACE_TEST_DATA "/dives/suunto_nautic_sidemount.xml");
}

void TestParse::importSuuntoJsonNauticMultigas()
{
#if defined(SUBSURFACE_MOBILE)
	QSKIP("Not testing Suunto JSON import on SUBSURFACE_MOBILE");
#endif
	/* Suunto Nautic, two reported gases (air and EAN50), one tank pod per
	 * reported gas and a gas change in the middle of the dive. The test data
	 * is derived
	 * from suunto_nautic_sidemount.json, trimmed to the first 1800
	 * samples and given a second gas with a second tank pod. Gas 1 is
	 * configured but not reported.
	 *
	 * The Gases array is deliberately in the reverse order from the
	 * gas switches. The gas mix, tank size and pressures have to end
	 * up in the cylinder with the matching sparse gas number, the
	 * unreported cylinder must remain a placeholder, and the gas change
	 * event has to refer to the reported cylinder. */
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/suunto_nautic_multigas.json", &divelog), 0);
	QCOMPARE(divelog.dives.size(), 1);

	auto &dive = divelog.dives[0];
	QCOMPARE(dive->cylinders.size(), 3);

	/* air on the first cylinder */
	QCOMPARE(dive->cylinders[0].gasmix.o2.permille, 0);
	QCOMPARE(dive->cylinders[0].type.size.mliter, 12000);
	QCOMPARE(dive->cylinders[0].type.workingpressure.mbar, 200000);
	QCOMPARE(dive->cylinders[0].start.mbar, 200000);

	/* The configured but unreported gas must not be imported. */
	QCOMPARE(dive->cylinders[1].gasmix.o2.permille, 0);
	QCOMPARE(dive->cylinders[1].type.size.mliter, 0);

	/* EAN50 on the sparse third cylinder */
	QCOMPARE(dive->cylinders[2].gasmix.o2.permille, 500);
	QCOMPARE(dive->cylinders[2].type.size.mliter, 11000);
	QCOMPARE(dive->cylinders[2].type.workingpressure.mbar, 232000);
	QCOMPARE(dive->cylinders[2].start.mbar, 232000);

	/* the gas change in the middle of the dive has to refer to the
	 * sparse third cylinder */
	auto gas_change = std::find_if(dive->dcs[0].events.begin(), dive->dcs[0].events.end(),
		[](const auto &ev) { return ev.time.seconds == 454 && ev.is_gaschange(); });
	QVERIFY(gas_change != dive->dcs[0].events.end());
	QCOMPARE(gas_change->gas.index, 2);

	QCOMPARE(save_dives("./test_suunto_nautic_multigas.ssrf"), 0);
	FILE_COMPARE("./test_suunto_nautic_multigas.ssrf",
		SUBSURFACE_TEST_DATA "/dives/suunto_nautic_multigas.xml");
}

void TestParse::importSuuntoJsonEonCore()
{
#if defined(SUBSURFACE_MOBILE)
	QSKIP("Not testing Suunto JSON import on SUBSURFACE_MOBILE");
#endif
	/* Suunto EON Core, single cylinder EAN32, 12 L tank, transmitter.
	 * Tests the EON JSON variant (Events array, 1-based GasNumber). */
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/suunto_eon_core_nitrox.json", &divelog), 0);
	QCOMPARE(save_dives("./test_suunto_eon_core_nitrox.ssrf"), 0);
	FILE_COMPARE("./test_suunto_eon_core_nitrox.ssrf",
		SUBSURFACE_TEST_DATA "/dives/suunto_eon_core_nitrox.xml");
}

void TestParse::importSuuntoJsonOcean()
{
#if defined(SUBSURFACE_MOBILE)
	QSKIP("Not testing Suunto JSON import on SUBSURFACE_MOBILE");
#endif
	/* Suunto Ocean (codename Porvoo), air dive with tank transmitter on gas 0. */
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/suunto_ocean_air.json", &divelog), 0);
	QCOMPARE(divelog.dives.size(), 1);
	QCOMPARE(divelog.dives[0]->dcs[0].when, divelog.dives[0]->when);
	QCOMPARE(save_dives("./test_suunto_ocean_air.ssrf"), 0);
	FILE_COMPARE("./test_suunto_ocean_air.ssrf",
		SUBSURFACE_TEST_DATA "/dives/suunto_ocean_air.xml");
}

// AI-generated (Claude)
void TestParse::importSuuntoJsonOceanNoFit()
{
#if defined(SUBSURFACE_MOBILE)
	QSKIP("Not testing Suunto JSON import on SUBSURFACE_MOBILE");
#else
	/* Regression guard: calling suunto_json_import() directly with an empty
	 * fit_buffer (i.e. no paired FIT file selected) must behave exactly
	 * like the plain JSON-only import above, reusing the same reference
	 * output. This confirms the added parameter didn't change the default
	 * (no-FIT) behavior. suunto_json_import() itself is only declared for
	 * !SUBSURFACE_MOBILE (see core/file.h), so this whole body -- not just
	 * the QSKIP above -- has to be compiled out on mobile. */
	auto [json_buf, jerr] = readfile(SUBSURFACE_TEST_DATA "/dives/suunto_ocean_air.json");
	QVERIFY(jerr > 0);
	QCOMPARE(suunto_json_import(json_buf, std::string(), &divelog), 1);
	QCOMPARE(save_dives("./test_suunto_ocean_air_no_fit.ssrf"), 0);
	FILE_COMPARE("./test_suunto_ocean_air_no_fit.ssrf",
		SUBSURFACE_TEST_DATA "/dives/suunto_ocean_air.xml");
#endif
}

void TestParse::importSuuntoJsonOceanWithFit()
{
#if defined(SUBSURFACE_MOBILE)
	QSKIP("Not testing Suunto JSON import on SUBSURFACE_MOBILE");
#else
	/* Suunto Ocean, EAN32 dive. The JSON alone has no gas mix (the Ocean's
	 * JSON export has no Diving.Gases block), so suunto_json_import() is
	 * given the paired FIT file too, which carries the gas mix (DIVE_GAS)
	 * and gradient factors (DIVE_SETTINGS) that get patched onto the
	 * JSON-derived dive. suunto_json_import() is only declared for
	 * !SUBSURFACE_MOBILE (see core/file.h), so this whole body -- not just
	 * the QSKIP above -- has to be compiled out on mobile. */
	auto [json_buf, jerr] = readfile(SUBSURFACE_TEST_DATA "/dives/suunto_ocean_nitrox.json");
	auto [fit_buf, ferr] = readfile(SUBSURFACE_TEST_DATA "/dives/suunto_ocean_nitrox.fit");
	QVERIFY(jerr > 0);
	QVERIFY(ferr > 0);
	QCOMPARE(suunto_json_import(json_buf, fit_buf, &divelog), 1);
	QCOMPARE(save_dives("./test_suunto_ocean_nitrox.ssrf"), 0);
	FILE_COMPARE("./test_suunto_ocean_nitrox.ssrf",
		SUBSURFACE_TEST_DATA "/dives/suunto_ocean_nitrox.xml");
#endif
}

#ifndef SUBSURFACE_MOBILE
namespace {

// AI-generated (Claude)
// Minimal implementation of the FIT CRC-16 algorithm from the Garmin FIT
// protocol. libdivecomputer's Garmin parser (src/garmin_parser.c) never
// checks this value -- it only checks that the file is at least as long as
// its declared header + data size -- but computing it properly keeps the
// buffers built below well-formed FIT files in their own right.
uint16_t fit_crc16(const unsigned char *data, size_t len)
{
	static const uint16_t table[16] = {
		0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
		0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
	};
	uint16_t crc = 0;
	for (size_t i = 0; i < len; ++i) {
		unsigned char byte = data[i];
		uint16_t tmp = table[crc & 0xF];
		crc = (crc >> 4) & 0x0FFF;
		crc = crc ^ tmp ^ table[byte & 0xF];
		tmp = table[crc & 0xF];
		crc = (crc >> 4) & 0x0FFF;
		crc = crc ^ tmp ^ table[(byte >> 4) & 0xF];
	}
	return crc;
}

struct fit_field { unsigned char num, size, base_type; };

void append_u8(std::vector<unsigned char> &b, unsigned char v)
{
	b.push_back(v);
}
void append_u16le(std::vector<unsigned char> &b, unsigned short v)
{
	b.push_back(v & 0xff);
	b.push_back((v >> 8) & 0xff);
}
void append_u32le(std::vector<unsigned char> &b, unsigned int v)
{
	for (int i = 0; i < 4; i++)
		b.push_back((v >> (8 * i)) & 0xff);
}

void append_definition(std::vector<unsigned char> &b, unsigned char local_type,
			unsigned short global_mesg, const std::vector<fit_field> &fields)
{
	append_u8(b, 0x40 | local_type);	// definition record (bit 6 set)
	append_u8(b, 0);			// reserved
	append_u8(b, 0);			// architecture: little-endian
	append_u16le(b, global_mesg);
	append_u8(b, (unsigned char)fields.size());
	for (const fit_field &f : fields) {
		append_u8(b, f.num);
		append_u8(b, f.size);
		append_u8(b, f.base_type);
	}
}

void begin_data_record(std::vector<unsigned char> &b, unsigned char local_type)
{
	append_u8(b, local_type);	// normal data record (bits 6 and 7 clear)
}

// FIT base-type byte values used below. The low 7 bits of each must match
// the index libdivecomputer's base_type_info[] table (src/garmin_parser.c)
// uses after masking off bit 7, which is all that parser looks at.
constexpr unsigned char FIT_ENUM = 0x00;
constexpr unsigned char FIT_UINT8 = 0x02;
constexpr unsigned char FIT_UINT16 = 0x84;

// AI-generated (Claude)
// Builds a minimal, hand-constructed Garmin FIT buffer carrying exactly the
// messages that libdivecomputer's src/garmin_parser.c decodes into
// DC_FIELD_GASMIX and DC_FIELD_TANK: one SPORT message (drives
// DC_FIELD_DIVEMODE), one DIVE_GAS message per entry in gas_o2_percent
// (open-circuit, enabled gas mixes at message_index 0, 1, ...), and one
// SENSOR_PROFILE "tank pod" message (sensor_type 28) per entry in
// tank_used_for_gas_rate, with field 78 (used_for_gas_rate) set to 1
// (-> DC_TANK_USAGE_INCLUDED) or 0 (-> DC_TANK_USAGE_EXCLUDED). No other
// record types (samples, GPS, device_info, ...) are included.
//
// This is used instead of a downloaded device fixture because no sample
// under tests/dives, nor any sample in the libdivecomputer test/sample
// corpus, carries a Garmin SENSOR_PROFILE record with the "used for gas
// rate" flag combinations this test needs (see the provenance comments on
// the two tests that call this below). The resulting buffer is fed to the
// real fit_file_import() entry point, so it exercises parse_gasmixes()'s
// actual call to cylinder_use_from_tank_usage() in core/libdivecomputer.cpp,
// not a direct call to that helper.
std::string build_garmin_fit_buffer(const std::vector<unsigned char> &gas_o2_percent,
				     const std::vector<unsigned char> &tank_used_for_gas_rate,
				     unsigned char sub_sport)
{
	std::vector<unsigned char> body;

	// SPORT (global mesg 12): sub_sport 53 (single-gas) or 54 (multi-gas)
	// both map to DC_DIVEMODE_OC in garmin_parser.c's parse_SPORT_sub_sport().
	append_definition(body, 0, 12, {{1, 1, FIT_ENUM}});
	begin_data_record(body, 0);
	append_u8(body, sub_sport);

	// DIVE_GAS (global mesg 259): one open-circuit, enabled gas mix per
	// entry, identified by the "message_index" field (254).
	append_definition(body, 1, 259, {
		{254, 2, FIT_UINT16},	// message_index
		{0, 1, FIT_UINT8},	// helium percent
		{1, 1, FIT_UINT8},	// oxygen percent
		{2, 1, FIT_ENUM},	// status: 1 == enabled
		{3, 1, FIT_ENUM},	// type: 0 == open circuit
	});
	for (size_t i = 0; i < gas_o2_percent.size(); i++) {
		begin_data_record(body, 1);
		append_u16le(body, (unsigned short)i);
		append_u8(body, 0);
		append_u8(body, gas_o2_percent[i]);
		append_u8(body, 1);
		append_u8(body, 0);
	}

	// SENSOR_PROFILE (global mesg 147): one tank-pod record per entry.
	append_definition(body, 2, 147, {
		{52, 1, FIT_ENUM},	// sensor_type: 28 == tank pod
		{78, 1, FIT_ENUM},	// used_for_gas_rate
	});
	for (unsigned char used : tank_used_for_gas_rate) {
		begin_data_record(body, 2);
		append_u8(body, 28);
		append_u8(body, used);
	}

	std::vector<unsigned char> file;
	append_u8(file, 12);					// header size
	append_u8(file, 0x10);					// protocol version
	append_u16le(file, 0);					// profile version (ignored by this parser)
	append_u32le(file, (unsigned int)body.size());	// data size
	file.insert(file.end(), {'.', 'F', 'I', 'T'});
	file.insert(file.end(), body.begin(), body.end());

	uint16_t crc = fit_crc16(file.data(), file.size());
	append_u16le(file, crc);

	return std::string(reinterpret_cast<const char *>(file.data()), file.size());
}

} // namespace

// AI-generated (Claude)
void TestParse::importGarminFit()
{
	auto [buffer, err] = readfile(SUBSURFACE_TEST_DATA "/dives/garmin_2023-10-21-12-13-38.fit");
	QVERIFY(err > 0);
	QCOMPARE(fit_file_import(buffer, &divelog), 1);
	QCOMPARE(divelog.dives.size(), 1);
	QCOMPARE(QString::fromStdString(divelog.dives[0]->dcs[0].model),
		QString::fromUtf8("Garmin Descent™ Mk1 (Imported from file)"));

	/* This fixture carries no DC_FIELD_TANK data at all (libdivecomputer
	 * reports DC_FIELD_TANK_COUNT as unsupported for it), so none of the
	 * per-tank gas-usage flags (DC_TANK_USAGE_SIDEMOUNT/INCLUDED/EXCLUDED)
	 * that cylinder_use_from_tank_usage() in core/libdivecomputer.cpp acts
	 * on are present; cylinder_use below comes solely from the
	 * DC_FIELD_GASMIX usage field of this CCR dive. Neither the
	 * sidemount/INCLUDED promotion nor the EXCLUDED demotion in
	 * parse_gasmixes() fires for this fixture. These assertions pin the
	 * observed cylinder count and uses to catch an accidental change to
	 * that. */
	QCOMPARE(divelog.dives[0]->cylinders.size(), 4u);
	QCOMPARE(divelog.dives[0]->cylinders[0].cylinder_use, DILUENT);
	QCOMPARE(divelog.dives[0]->cylinders[1].cylinder_use, OC_GAS);
	QCOMPARE(divelog.dives[0]->cylinders[2].cylinder_use, OC_GAS);
	QCOMPARE(divelog.dives[0]->cylinders[3].cylinder_use, DILUENT);
}

// AI-generated (Claude)
void TestParse::importGarminFitSidemountPromotesSecondTank()
{
	/* Synthetic, hand-built Garmin FIT buffer -- see the provenance
	 * comment on build_garmin_fit_buffer() above for why. This buffer
	 * carries a single open-circuit gas mix and two SENSOR_PROFILE tank
	 * pods, both flagged "used for gas rate" (-> DC_TANK_USAGE_INCLUDED),
	 * the real-device equivalent of a two-cylinder, single-mix sidemount
	 * rig. The first tank's cylinder_use already comes from its own
	 * DC_FIELD_GASMIX; the second tank has no gas mix of its own, so this
	 * confirms parse_gasmixes() passes its real ngases==1, ntanks==2
	 * values into cylinder_use_from_tank_usage(), which then promotes it
	 * from NOT_USED to OC_GAS, and that parse_gasmixes() applies that
	 * result to the imported cylinder. */
	std::string buffer = build_garmin_fit_buffer({21}, {1, 1}, 53);

	/* A single gas mix with two tank pods (ngases=1, ntanks=2) is exactly
	 * the sidemount/INCLUDED-promotion shape parse_gasmixes() is meant to
	 * handle, but parse_gasmixes() also always logs a "smaller number of
	 * gases... than cylinders" report_error() whenever ntanks > ngases,
	 * by design, not as a bug (core/libdivecomputer.cpp). testbase.cpp
	 * installs a callback that fails the test on any report_error(), so
	 * swap in a no-op for this one import and restore the normal
	 * fail-on-error behaviour immediately after. */
	set_error_cb([](std::string) {});
	int ret = fit_file_import(buffer, &divelog);
	set_error_cb(&TestBase::failOnError);

	QCOMPARE(ret, 1);
	QCOMPARE(divelog.dives.size(), 1);
	QCOMPARE(divelog.dives[0]->cylinders.size(), 2u);
	QCOMPARE(divelog.dives[0]->cylinders[0].cylinder_use, OC_GAS);
	QCOMPARE(divelog.dives[0]->cylinders[1].cylinder_use, OC_GAS);
}

// AI-generated (Claude)
void TestParse::importGarminFitExcludedOverridesGasmix()
{
	/* Synthetic, hand-built Garmin FIT buffer -- see the provenance
	 * comment on build_garmin_fit_buffer() above for why. This buffer
	 * carries two open-circuit gas mixes and two SENSOR_PROFILE tank
	 * pods, the second one flagged "not used for gas rate"
	 * (-> DC_TANK_USAGE_EXCLUDED), the real-device equivalent of a
	 * monitored but not consumption-counted tank (e.g. a buddy/stage
	 * transmitter). Both tanks get a cylinder_use from their own
	 * DC_FIELD_GASMIX first; this confirms parse_gasmixes() still passes
	 * the second tank's real DC_TANK_USAGE_EXCLUDED flag into
	 * cylinder_use_from_tank_usage(), which demotes it to NOT_USED despite
	 * that gasmix-derived use, and that parse_gasmixes() applies that
	 * result rather than keeping the gasmix-derived one. */
	std::string buffer = build_garmin_fit_buffer({21, 50}, {1, 0}, 54);
	QCOMPARE(fit_file_import(buffer, &divelog), 1);
	QCOMPARE(divelog.dives.size(), 1);
	QCOMPARE(divelog.dives[0]->cylinders.size(), 2u);
	QCOMPARE(divelog.dives[0]->cylinders[0].cylinder_use, OC_GAS);
	QCOMPARE(divelog.dives[0]->cylinders[1].cylinder_use, NOT_USED);
}
#endif

QTEST_GUILESS_MAIN(TestParse)
