// SPDX-License-Identifier: GPL-2.0
#include "testqPrefDisplay.h"

#include "core/settings/qPrefDisplay.h"

#include <QtTest>
#include <QDate>

#define TEST(METHOD, VALUE) \
QCOMPARE(METHOD, VALUE); \
display->sync(); \
display->load(); \
QCOMPARE(METHOD, VALUE);

void TestQPrefDisplay::initTestCase()
{
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefDisplay");
}

void TestQPrefDisplay::test1()
{
	auto display = qPrefDisplay::instance();
	display->set_divelist_font("comic");
	display->set_font_size(10.0);
	display->set_display_invalid_dives(true);

	TEST(display->divelist_font(),QStringLiteral("comic"));
	TEST(display->font_size(), 10.0);
	TEST(display->display_invalid_dives(), true);

	display->set_divelist_font("helvetica");
	display->set_font_size(14.0);
	display->set_display_invalid_dives(false);

	TEST(display->divelist_font(),QStringLiteral("helvetica"));
	TEST(display->font_size(), 14.0);
	TEST(display->display_invalid_dives(), false);
}

QTEST_MAIN(TestQPrefDisplay)
