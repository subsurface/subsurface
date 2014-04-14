#include "testunitconversion.h"
#include "dive.h"

void TestUnitConversion::testUnitConversions()
{
	QCOMPARE(IS_FP_SAME(grams_to_lbs(1000), 2.20459), true);
	QCOMPARE(lbs_to_grams(1), 454);
	QCOMPARE(IS_FP_SAME(ml_to_cuft(1000), 0.0353147), true);
	QCOMPARE(IS_FP_SAME(cuft_to_l(1), 28.3168), true);
	QCOMPARE(IS_FP_SAME(mm_to_feet(1000), 3.28084), true);
	QCOMPARE(feet_to_mm(1), (long unsigned int) 305);
}

QTEST_MAIN(TestUnitConversion)