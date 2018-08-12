// SPDX-License-Identifier: GPL-2.0
#include "testpreferences.h"

#include "core/subsurface-qt/SettingsObjectWrapper.h"

#include <QDate>
#include <QtTest>

#define TEST(METHOD, VALUE)      \
	QCOMPARE(METHOD, VALUE); \
	pref->sync();            \
	pref->load();            \
	QCOMPARE(METHOD, VALUE);

void TestPreferences::initTestCase()
{
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestPreferences");
}

void TestPreferences::testPreferences()
{
	auto pref = SettingsObjectWrapper::instance();
	pref->load();

	auto general = pref->general_settings;
	general->setDefaultFilename("filename");
	general->setDefaultCylinder("cylinder_2");
	//TODOl: Change this to a enum. 	// This is 'undefined', it will need to figure out later between no_file or use_deault file.
	general->set_default_file_behavior(LOCAL_DEFAULT_FILE);
	general->set_defaultsetpoint(0);
	general->set_o2consumption(0);
	general->set_pscr_ratio(0);
	general->set_use_default_file(true);

	TEST(general->default_filename(), QStringLiteral("filename"));
	TEST(general->default_cylinder(), QStringLiteral("cylinder_2"));
	TEST(general->default_file_behavior(), LOCAL_DEFAULT_FILE); // since we have a default file, here it returns
	TEST(general->defaultsetpoint(), 0);
	TEST(general->o2consumption(), 0);
	TEST(general->pscr_ratio(), 0);
	TEST(general->use_default_file(), true);

	general->set_default_filename("no_file_name");
	general->set_default_cylinder("cylinder_1");
	//TODOl: Change this to a enum.
	general->set_default_file_behavior(CLOUD_DEFAULT_FILE);

	general->set_defaultsetpoint(1);
	general->set_o2consumption(1);
	general->set_pscr_ratio(1);
	general->set_use_default_file(false);

	TEST(general->default_filename(), QStringLiteral("no_file_name"));
	TEST(general->default_cylinder(), QStringLiteral("cylinder_1"));
	TEST(general->default_file_behavior(), CLOUD_DEFAULT_FILE);
	TEST(general->defaultsetpoint(), 1);
	TEST(general->o2consumption(), 1);
	TEST(general->pscr_ratio(), 1);
	TEST(general->use_default_file(), false);
}

QTEST_MAIN(TestPreferences)
