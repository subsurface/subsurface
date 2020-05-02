// SPDX-License-Identifier: GPL-2.0
#include "testprofile.h"
#include "core/divesite.h"
#include "core/trip.h"
#include "core/file.h"
#include "core/save-profiledata.h"

void TestProfile::testProfileExport()
{
	parse_file("../dives/abitofeverything.ssrf", &dive_table, &trip_table, &dive_site_table);
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
