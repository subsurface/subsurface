#include "testprofile.h"
#include "dive.h"

void TestProfile::testRedCeiling()
{
	parse_file("../dives/deep.xml");
}

QTEST_MAIN(TestProfile)