// SPDX-License-Identifier: GPL-2.0
#include "testprofile.h"
#include "core/device.h"
#include "core/dive.h"
#include "core/divelog.h"
#include "core/divesite.h"
#include "core/trip.h"
#include "core/file.h"
#include "core/save-profiledata.h"
#include "core/pref.h"
#include "QTextCodec"

// This test compares the content of struct profile against a known reference version for a list
// of dives to prevent accidental regressions. Thus is you change anything in the profile this
// test will fail. If this change was intentional, run the test manually. Make sure only the
// indended fields change (for example by computing a diff between exportprofile.csv and
// ..dives/exportprofilereference.csv) and copy the former over the later and commit that change
// as well.

void TestProfile::init()
{
	setlocale(LC_ALL, "C");

	// Set UTF8 text codec as in real applications
	QTextCodec::setCodecForLocale(QTextCodec::codecForMib(106));

	// first, setup the preferences
	prefs = default_prefs;

	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("Subsurface");
}
void TestProfile::testProfileExport()
{
	prefs.planner_deco_mode = BUEHLMANN;
	parse_file(SUBSURFACE_TEST_DATA "/dives/abitofeverything.ssrf", &divelog);
	save_profiledata("exportprofile.csv", false);
	QFile org(SUBSURFACE_TEST_DATA "/dives/exportprofilereference.csv");
	QCOMPARE(org.open(QFile::ReadOnly), true);
	QFile out("exportprofile.csv");
	QCOMPARE(out.open(QFile::ReadOnly), true);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);

}
void TestProfile::testProfileExportVPMB()
{
	prefs.planner_deco_mode = VPMB;
	parse_file(SUBSURFACE_TEST_DATA "/dives/abitofeverything.ssrf", &divelog);
	save_profiledata("exportprofileVPMB.csv", false);
	QFile org(SUBSURFACE_TEST_DATA "/dives/exportprofilereferenceVPMB.csv");
	QCOMPARE(org.open(QFile::ReadOnly), true);
	QFile out("exportprofileVPMB.csv");
	QCOMPARE(out.open(QFile::ReadOnly), true);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);

}

QTEST_GUILESS_MAIN(TestProfile)
