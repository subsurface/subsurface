// SPDX-License-Identifier: GPL-2.0
#include "testdivesiteduplication.h"
#include "core/divelog.h"
#include "core/divesite.h"
#include "core/file.h"
#include "core/pref.h"

void TestDiveSiteDuplication::testReadV2()
{
	prefs.cloud_base_url = default_prefs.cloud_base_url;
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/TwoTimesTwo.ssrf", &divelog), 0);
	QCOMPARE(divelog.sites.size(), 2);
}

QTEST_GUILESS_MAIN(TestDiveSiteDuplication)
