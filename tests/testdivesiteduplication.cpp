#include "testdivesiteduplication.h"
#include "core/dive.h"
#include "core/divesite.h"

void TestDiveSiteDuplication::testReadV2()
{
	QCOMPARE(parse_file(SUBSURFACE_SOURCE "/dives/TwoTimesTwo.ssrf"), 0);
	QCOMPARE(dive_site_table.nr, 2);
}

QTEST_GUILESS_MAIN(TestDiveSiteDuplication)
