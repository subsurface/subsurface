#include "testrenumber.h"
#include "dive.h"
#include "file.h"
#include "divelist.h"
#include <QTextStream>

void TestRenumber::setup()
{
	QCOMPARE(parse_file(SUBSURFACE_SOURCE "/dives/test47.xml"), 0);
	process_dives(false, false);
	dive_table.preexisting = dive_table.nr;
}

void TestRenumber::testMerge()
{
	QCOMPARE(parse_file(SUBSURFACE_SOURCE "/dives/test47b.xml"), 0);
	process_dives(true, false);
	QCOMPARE(dive_table.nr, 1);
	QCOMPARE(unsaved_changes(), 1);
	mark_divelist_changed(false);
	dive_table.preexisting = dive_table.nr;
}

void TestRenumber::testMergeAndAppend()
{
	QCOMPARE(parse_file(SUBSURFACE_SOURCE "/dives/test47c.xml"), 0);
	process_dives(true, false);
	QCOMPARE(dive_table.nr, 2);
	QCOMPARE(unsaved_changes(), 1);
	struct dive *d = get_dive(1);
	QVERIFY(d != NULL);
	if (d)
		QCOMPARE(d->number, 2);
}

QTEST_MAIN(TestRenumber)
