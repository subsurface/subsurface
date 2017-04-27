// SPDX-License-Identifier: GPL-2.0
#include "testprofile.h"
#include "core/dive.h"

void TestProfile::testRedCeiling()
{
	parse_file("../dives/deep.xml");
}

QTEST_GUILESS_MAIN(TestProfile)
