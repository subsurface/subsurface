// SPDX-License-Identifier: GPL-2.0
#include "testprofile.h"
#include "core/divesite.h"
#include "core/trip.h"
#include "core/file.h"

void TestProfile::testRedCeiling()
{
	parse_file("../dives/deep.xml", &dive_table, &trip_table, &dive_site_table);
}

QTEST_GUILESS_MAIN(TestProfile)
