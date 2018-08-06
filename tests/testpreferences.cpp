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

	auto pp = pref->pp_gas;
	pp->setShowPn2(false);
	pp->setShowPhe(false);
	pp->setShowPo2(false);
	pp->setPo2ThresholdMin(1.0);
	pp->setPo2ThresholdMax(2.0);
	pp->setPn2Threshold(3.0);
	pp->setPheThreshold(4.0);

	TEST(pp->showPn2(), false);
	TEST(pp->showPhe(), false);
	TEST(pp->showPo2(), false);
	TEST(pp->pn2Threshold(), 3.0);
	TEST(pp->pheThreshold(), 4.0);
	TEST(pp->po2ThresholdMin(), 1.0);
	TEST(pp->po2ThresholdMax(), 2.0);

	pp->setShowPn2(true);
	pp->setShowPhe(true);
	pp->setShowPo2(true);
	pp->setPo2ThresholdMin(4.0);
	pp->setPo2ThresholdMax(5.0);
	pp->setPn2Threshold(6.0);
	pp->setPheThreshold(7.0);

	TEST(pp->showPn2(), true);
	TEST(pp->showPhe(), true);
	TEST(pp->showPo2(), true);
	TEST(pp->pn2Threshold(), 6.0);
	TEST(pp->pheThreshold(), 7.0);
	TEST(pp->po2ThresholdMin(), 4.0);
	TEST(pp->po2ThresholdMax(), 5.0);

	auto geo = pref->geocoding;
	geo->setFirstTaxonomyCategory(TC_NONE);
	geo->setSecondTaxonomyCategory(TC_OCEAN);
	geo->setThirdTaxonomyCategory(TC_COUNTRY);

	TEST(geo->firstTaxonomyCategory(), TC_NONE);
	TEST(geo->secondTaxonomyCategory(), TC_OCEAN);
	TEST(geo->thirdTaxonomyCategory(), TC_COUNTRY);

	geo->setFirstTaxonomyCategory(TC_OCEAN);
	geo->setSecondTaxonomyCategory(TC_COUNTRY);
	geo->setThirdTaxonomyCategory(TC_NONE);

	TEST(geo->firstTaxonomyCategory(), TC_OCEAN);
	TEST(geo->secondTaxonomyCategory(), TC_COUNTRY);
	TEST(geo->thirdTaxonomyCategory(), TC_NONE);

	auto general = pref->general_settings;
	general->setDefaultFilename("filename");
	general->setDefaultCylinder("cylinder_2");
	//TODOl: Change this to a enum. 	// This is 'undefined', it will need to figure out later between no_file or use_deault file.
	general->setDefaultFileBehavior(0);
	general->setDefaultSetPoint(0);
	general->setO2Consumption(0);
	general->setPscrRatio(0);
	general->setUseDefaultFile(true);

	TEST(general->defaultFilename(), QStringLiteral("filename"));
	TEST(general->defaultCylinder(), QStringLiteral("cylinder_2"));
	TEST(general->defaultFileBehavior(), (short)LOCAL_DEFAULT_FILE); // since we have a default file, here it returns
	TEST(general->defaultSetPoint(), 0);
	TEST(general->o2Consumption(), 0);
	TEST(general->pscrRatio(), 0);
	TEST(general->useDefaultFile(), true);

	general->setDefaultFilename("no_file_name");
	general->setDefaultCylinder("cylinder_1");
	//TODOl: Change this to a enum.
	general->setDefaultFileBehavior(CLOUD_DEFAULT_FILE);

	general->setDefaultSetPoint(1);
	general->setO2Consumption(1);
	general->setPscrRatio(1);
	general->setUseDefaultFile(false);

	TEST(general->defaultFilename(), QStringLiteral("no_file_name"));
	TEST(general->defaultCylinder(), QStringLiteral("cylinder_1"));
	TEST(general->defaultFileBehavior(), (short)CLOUD_DEFAULT_FILE);
	TEST(general->defaultSetPoint(), 1);
	TEST(general->o2Consumption(), 1);
	TEST(general->pscrRatio(), 1);
	TEST(general->useDefaultFile(), false);

	auto language = pref->language_settings;
	language->setLangLocale("en_US");
	language->setLanguage("en");
	language->setTimeFormat("hh:mm");
	language->setDateFormat("dd/mm/yy");
	language->setDateFormatShort("dd/mm");
	language->setTimeFormatOverride(false);
	language->setDateFormatOverride(false);
	language->setUseSystemLanguage(false);

	TEST(language->langLocale(), QStringLiteral("en_US"));
	TEST(language->language(), QStringLiteral("en"));
	TEST(language->timeFormat(), QStringLiteral("hh:mm"));
	TEST(language->dateFormat(), QStringLiteral("dd/mm/yy"));
	TEST(language->dateFormatShort(), QStringLiteral("dd/mm"));
	TEST(language->timeFormatOverride(), false);
	TEST(language->dateFormatOverride(), false);
	TEST(language->useSystemLanguage(), false);

	language->setLangLocale("en_EN");
	language->setLanguage("br");
	language->setTimeFormat("mm:hh");
	language->setDateFormat("yy/mm/dd");
	language->setDateFormatShort("dd/yy");
	language->setTimeFormatOverride(true);
	language->setDateFormatOverride(true);
	language->setUseSystemLanguage(true);

	TEST(language->langLocale(), QStringLiteral("en_EN"));
	TEST(language->language(), QStringLiteral("br"));
	TEST(language->timeFormat(), QStringLiteral("mm:hh"));
	TEST(language->dateFormat(), QStringLiteral("yy/mm/dd"));
	TEST(language->dateFormatShort(), QStringLiteral("dd/yy"));
	TEST(language->timeFormatOverride(), true);
	TEST(language->dateFormatOverride(), true);
	TEST(language->useSystemLanguage(), true);

	auto location = qPrefLocationService::instance();
	location->set_time_threshold(10);
	location->set_distance_threshold(20);

	TEST(location->time_threshold(), 10);
	TEST(location->distance_threshold(), 20);

	location->set_time_threshold(30);
	location->set_distance_threshold(40);

	TEST(location->timeThreshold(), 30);
	TEST(location->distanceThreshold(), 40);
}

QTEST_MAIN(TestPreferences)
