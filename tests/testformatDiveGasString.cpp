#include "testformatDiveGasString.h"

#include "../core/dive.h"
#include "../core/string-format.h"

void TestformatDiveGasString::init()
{
}

void TestformatDiveGasString::test_empty()
{
	struct dive dive;

	QCOMPARE(formatDiveGasString(&dive), "air");
}

void TestformatDiveGasString::test_air()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	QCOMPARE(formatDiveGasString(&dive), "air");
}

void TestformatDiveGasString::test_nitrox()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2.permille = 320;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	QCOMPARE(formatDiveGasString(&dive), "32%");
}

void TestformatDiveGasString::test_nitrox_not_use()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2.permille = 320;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	cylinder = dive.get_or_create_cylinder(1);

	cylinder->gasmix.o2.permille = 1000;
	cylinder->cylinder_use = NOT_USED;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	QCOMPARE(formatDiveGasString(&dive), "32%");
}

void TestformatDiveGasString::test_nitrox_deco()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2.permille = 320;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	cylinder = dive.get_or_create_cylinder(1);

	cylinder->gasmix.o2.permille = 1000;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	QCOMPARE(formatDiveGasString(&dive), "32…100%");
}

void TestformatDiveGasString::test_reverse_nitrox_deco()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2.permille = 1000;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	cylinder = dive.get_or_create_cylinder(1);

	cylinder->gasmix.o2.permille = 270;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	QCOMPARE(formatDiveGasString(&dive), "27…100%");
}

void TestformatDiveGasString::test_trimix()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2.permille = 210;
	cylinder->gasmix.he.permille = 350;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	QCOMPARE(formatDiveGasString(&dive), "21/35");
}

void TestformatDiveGasString::test_trimix_deco()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2.permille = 210;
	cylinder->gasmix.he.permille = 350;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	cylinder = dive.get_or_create_cylinder(1);

	cylinder->gasmix.o2.permille = 500;
	cylinder->gasmix.he.permille = 200;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	cylinder = dive.get_or_create_cylinder(2);

	cylinder->gasmix.o2.permille = 1000;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	QCOMPARE(formatDiveGasString(&dive), "21/35…100%");
}

void TestformatDiveGasString::test_reverse_trimix_deco()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2.permille = 1000;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	cylinder = dive.get_or_create_cylinder(1);

	cylinder->gasmix.o2.permille = 500;
	cylinder->gasmix.he.permille = 200;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	cylinder = dive.get_or_create_cylinder(2);

	cylinder->gasmix.o2.permille = 210;
	cylinder->gasmix.he.permille = 350;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	QCOMPARE(formatDiveGasString(&dive), "21/35…100%");
}

void TestformatDiveGasString::test_trimix_and_nitrox_same_o2()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2.permille = 250;
	cylinder->gasmix.he.permille = 0;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	cylinder = dive.get_or_create_cylinder(1);

	cylinder->gasmix.o2.permille = 250;
	cylinder->gasmix.he.permille = 250;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	QCOMPARE(formatDiveGasString(&dive), "25/25");
}

void TestformatDiveGasString::test_trimix_and_nitrox_lower_o2()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2.permille = 220;
	cylinder->gasmix.he.permille = 0;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	cylinder = dive.get_or_create_cylinder(1);

	cylinder->gasmix.o2.permille = 250;
	cylinder->gasmix.he.permille = 250;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	QCOMPARE(formatDiveGasString(&dive), "25/25");
}

void TestformatDiveGasString::test_ccr()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2.permille = 1000;
	cylinder->cylinder_use = OXYGEN;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	cylinder = dive.get_or_create_cylinder(1);

	cylinder->gasmix.o2.permille = 210;
	cylinder->gasmix.he.permille = 350;
	cylinder->cylinder_use = DILUENT;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	QCOMPARE(formatDiveGasString(&dive), "21/35");
}

void TestformatDiveGasString::test_ccr_bailout()
{
	struct dive dive;
	cylinder_t *cylinder = dive.get_or_create_cylinder(0);

	cylinder->gasmix.o2.permille = 1000;
	cylinder->cylinder_use = OXYGEN;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	cylinder = dive.get_or_create_cylinder(1);

	cylinder->gasmix.o2.permille = 220;
	cylinder->gasmix.he.permille = 200;
	cylinder->cylinder_use = DILUENT;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	cylinder = dive.get_or_create_cylinder(2);

	cylinder->gasmix.o2.permille = 210;
	cylinder->gasmix.he.permille = 0;
	cylinder->start.mbar = 230000;
	cylinder->end.mbar = 100000;

	QCOMPARE(formatDiveGasString(&dive), "22/20");
}

QTEST_GUILESS_MAIN(TestformatDiveGasString)
