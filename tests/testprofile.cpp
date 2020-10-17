// SPDX-License-Identifier: GPL-2.0
#include "testprofile.h"
#include "core/device.h"
#include "core/divesite.h"
#include "core/trip.h"
#include "core/file.h"
#include "core/save-profiledata.h"

// This test compares the content of struct profile against a known reference version for a list
// of dives to prevent accidental regressions. Thus is you change anything in the profile this
// test will fail. If this change was intentional, run the test manually. Make sure only the
// indended fields change (for example by computing a diff between exportprofile.csv and
// ..dives/exportprofilereference.csv) and copy the former over the later and commit that change
// as well.

void TestProfile::testProfileExport()
{
	parse_file("../dives/abitofeverything.ssrf", &dive_table, &trip_table, &dive_site_table, &device_table, &filter_preset_table);
	save_profiledata("exportprofile.csv", false);
	QFile org("../dives/exportprofilereference.csv");
	org.open(QFile::ReadOnly);
	QFile out("exportprofile.csv");
	out.open(QFile::ReadOnly);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);

}

QTEST_GUILESS_MAIN(TestProfile)
