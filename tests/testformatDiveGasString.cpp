#include "testformatDiveGasString.h"

#include "../core/dive.h"
#include "../core/string-format.h"

void TestformatDiveGasString::test_empty()
{
	struct dive dive;

	QCOMPARE(formatDiveGasString(&dive), "air");
}

void TestformatDiveGasString::test_air()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	QCOMPARE(formatDiveGasString(&dive), "air");
}

void TestformatDiveGasString::test_nitrox()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2 = 32_percent;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	QCOMPARE(formatDiveGasString(&dive), "32%");
}

void TestformatDiveGasString::test_nitrox_not_use()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2 = 32_percent;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	cylinder = dive.get_or_create_cylinder(1);

	cylinder->gasmix.o2 = 100_percent;
	cylinder->cylinder_use = NOT_USED;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	QCOMPARE(formatDiveGasString(&dive), "32%");
}

void TestformatDiveGasString::test_nitrox_deco()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2 = 32_percent;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	cylinder = dive.get_or_create_cylinder(1);

	cylinder->gasmix.o2 = 100_percent;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	QCOMPARE(formatDiveGasString(&dive), "32…100%");
}

void TestformatDiveGasString::test_reverse_nitrox_deco()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2 = 100_percent;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	cylinder = dive.get_or_create_cylinder(1);

	cylinder->gasmix.o2 = 27_percent;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	QCOMPARE(formatDiveGasString(&dive), "27…100%");
}

void TestformatDiveGasString::test_trimix()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2 = 21_percent;
	cylinder->gasmix.he = 35_percent;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	QCOMPARE(formatDiveGasString(&dive), "21/35");
}

void TestformatDiveGasString::test_trimix_deco()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2 = 21_percent;
	cylinder->gasmix.he = 35_percent;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	cylinder = dive.get_or_create_cylinder(1);

	cylinder->gasmix.o2 = 50_percent;
	cylinder->gasmix.he = 20_percent;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	cylinder = dive.get_or_create_cylinder(2);

	cylinder->gasmix.o2 = 100_percent;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	QCOMPARE(formatDiveGasString(&dive), "21/35…100%");
}

void TestformatDiveGasString::test_reverse_trimix_deco()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2 = 100_percent;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	cylinder = dive.get_or_create_cylinder(1);

	cylinder->gasmix.o2 = 50_percent;
	cylinder->gasmix.he = 20_percent;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	cylinder = dive.get_or_create_cylinder(2);

	cylinder->gasmix.o2 = 21_percent;
	cylinder->gasmix.he = 35_percent;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	QCOMPARE(formatDiveGasString(&dive), "21/35…100%");
}

void TestformatDiveGasString::test_trimix_and_nitrox_same_o2()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2 = 25_percent;
	cylinder->gasmix.he = 0_percent;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	cylinder = dive.get_or_create_cylinder(1);

	cylinder->gasmix.o2 = 25_percent;
	cylinder->gasmix.he = 25_percent;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	QCOMPARE(formatDiveGasString(&dive), "25/25");
}

void TestformatDiveGasString::test_trimix_and_nitrox_lower_o2()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2 = 22_percent;
	cylinder->gasmix.he = 0_percent;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	cylinder = dive.get_or_create_cylinder(1);

	cylinder->gasmix.o2 = 25_percent;
	cylinder->gasmix.he = 25_percent;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	QCOMPARE(formatDiveGasString(&dive), "25/25");
}

void TestformatDiveGasString::test_ccr()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2 = 100_percent;
	cylinder->cylinder_use = OXYGEN;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	cylinder = dive.get_or_create_cylinder(1);

	cylinder->gasmix.o2 = 21_percent;
	cylinder->gasmix.he = 35_percent;
	cylinder->cylinder_use = DILUENT;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	QCOMPARE(formatDiveGasString(&dive), "21/35");
}

void TestformatDiveGasString::test_ccr_bailout()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2 = 100_percent;
	cylinder->cylinder_use = OXYGEN;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	cylinder = dive.get_or_create_cylinder(1);

	cylinder->gasmix.o2 = 22_percent;
	cylinder->gasmix.he = 20_percent;
	cylinder->cylinder_use = DILUENT;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	cylinder = dive.get_or_create_cylinder(2);

	cylinder->gasmix.o2 = 21_percent;
	cylinder->gasmix.he = 0_percent;
	cylinder->start = 230_bar;
	cylinder->end = 100_bar;

	QCOMPARE(formatDiveGasString(&dive), "22/20");
}

QTEST_GUILESS_MAIN(TestformatDiveGasString)
