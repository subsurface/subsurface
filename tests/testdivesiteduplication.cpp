#include "testdivesiteduplication.h"
#include "dive.h"
#include "divesite.h"

void TestDiveSiteDuplication::testReadV2()
{
	QCOMPARE(parse_file(SUBSURFACE_SOURCE "/dives/TwoTimesTwo.ssrf"), 0);
	QCOMPARE(dive_site_table.nr, 2);
}

QTEST_MAIN(TestDiveSiteDuplication)
