// SPDX-License-Identifier: GPL-2.0
#include "testqPrefAnimations.h"

#include "core/settings/qPrefAnimations.h"
#include "core/pref.h"
#include "core/qthelper.h"

#include <QDate>

void TestQPrefAnimations::initTestCase()
{
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefAnimations");
}

void TestQPrefAnimations::test_struct_get()
{
	// Test struct pref -> get func.

	auto tst = qPrefAnimations::instance();

	prefs.animation_speed = 17;

	QCOMPARE(tst->animation_speed(), prefs.animation_speed);
}

void TestQPrefAnimations::test_set_struct()
{
	// Test set func -> struct pref

	auto tst = qPrefAnimations::instance();

	tst->set_animation_speed(27);

	QCOMPARE(prefs.animation_speed, 27);
}

void TestQPrefAnimations::test_set_load_struct()
{
	// test set func -> load -> struct pref 

	auto tst = qPrefAnimations::instance();

	tst->set_animation_speed(33);

	prefs.animation_speed = 17;

	tst->load();
	QCOMPARE(prefs.animation_speed, 33);
}

void TestQPrefAnimations::test_struct_disk()
{
	// test struct prefs -> disk

	auto tst = qPrefAnimations::instance();

	prefs.animation_speed = 27;

	tst->sync();
	prefs.animation_speed = 35;

	tst->load();
	QCOMPARE(prefs.animation_speed, 27);
}

void TestQPrefAnimations::test_multiple()
{
	// test multiple instances have the same information

	prefs.animation_speed = 37;
	auto tst_direct = new qPrefAnimations;

	prefs.animation_speed = 25;
	auto tst = qPrefAnimations::instance();

	QCOMPARE(tst->animation_speed(), tst_direct->animation_speed());
}

QTEST_MAIN(TestQPrefAnimations)
