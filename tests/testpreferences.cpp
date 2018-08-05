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

	auto tecDetails = qPrefTechnicalDetails::instance();
	tecDetails->set_modpO2(0.2);
	TEST(tecDetails->modpO2(), 0.2);
	tecDetails->set_modpO2(1.0);
	TEST(tecDetails->modpO2(), 1.0);

	tecDetails->set_gflow(2);
	TEST(tecDetails->gflow(), 2);
	tecDetails->set_gflow(3);
	TEST(tecDetails->gflow(), 3);

	tecDetails->set_gfhigh(4);
	TEST(tecDetails->gfhigh(), 4);
	tecDetails->set_gfhigh(5);
	TEST(tecDetails->gfhigh(), 5);

	tecDetails->set_vpmb_conservatism(5);
	TEST(tecDetails->vpmb_conservatism(), (short)5);
	tecDetails->set_vpmb_conservatism(6);
	TEST(tecDetails->vpmb_conservatism(), (short)6);

	tecDetails->set_ead(true);
	TEST(tecDetails->ead(), true);
	tecDetails->set_mod(true);
	TEST(tecDetails->mod(), true);
	tecDetails->set_dcceiling(true);
	TEST(tecDetails->dcceiling(), true);
	tecDetails->set_redceiling(true);
	TEST(tecDetails->redceiling(), true);
	tecDetails->set_calcceiling(true);
	TEST(tecDetails->calcceiling(), true);
	tecDetails->set_calcceiling3m(true);
	TEST(tecDetails->calcceiling3m(), true);
	tecDetails->set_calcalltissues(true);
	TEST(tecDetails->calcalltissues(), true);
	tecDetails->set_calcndltts(true);
	TEST(tecDetails->calcndltts(), true);
	tecDetails->set_hrgraph(true);
	TEST(tecDetails->hrgraph(), true);
	tecDetails->set_tankbar(true);
	TEST(tecDetails->tankbar(), true);
	tecDetails->set_percentagegraph(true);
	TEST(tecDetails->percentagegraph(), true);
	tecDetails->set_rulergraph(true);
	TEST(tecDetails->rulergraph(), true);
	tecDetails->set_show_ccr_setpoint(true);
	TEST(tecDetails->show_ccr_setpoint(), true);
	tecDetails->set_show_ccr_sensors(true);
	TEST(tecDetails->show_ccr_sensors(), true);
	tecDetails->set_zoomed_plot(true);
	TEST(tecDetails->zoomed_plot(), true);
	tecDetails->set_show_sac(true);
	TEST(tecDetails->show_sac(), true);
	tecDetails->set_display_unused_tanks(true);
	TEST(tecDetails->display_unused_tanks(), true);
	tecDetails->set_show_average_depth(true);
	TEST(tecDetails->show_average_depth(), true);
	tecDetails->set_show_pictures_in_profile(true);
	TEST(tecDetails->show_pictures_in_profile(), true);

	tecDetails->set_ead(false);
	TEST(tecDetails->ead(), false);
	tecDetails->set_mod(false);
	TEST(tecDetails->mod(), false);
	tecDetails->set_dcceiling(false);
	TEST(tecDetails->dcceiling(), false);
	tecDetails->set_redceiling(false);
	TEST(tecDetails->redceiling(), false);
	tecDetails->set_calcceiling(false);
	TEST(tecDetails->calcceiling(), false);
	tecDetails->set_calcceiling3m(false);
	TEST(tecDetails->calcceiling3m(), false);
	tecDetails->set_calcalltissues(false);
	TEST(tecDetails->calcalltissues(), false);
	tecDetails->set_calcndltts(false);
	TEST(tecDetails->calcndltts(), false);
	tecDetails->set_hrgraph(false);
	TEST(tecDetails->hrgraph(), false);
	tecDetails->set_tankbar(false);
	TEST(tecDetails->tankbar(), false);
	tecDetails->set_percentagegraph(false);
	TEST(tecDetails->percentagegraph(), false);
	tecDetails->set_rulergraph(false);
	TEST(tecDetails->rulergraph(), false);
	tecDetails->set_show_ccr_setpoint(false);
	TEST(tecDetails->show_ccr_setpoint(), false);
	tecDetails->set_show_ccr_sensors(false);
	TEST(tecDetails->show_ccr_sensors(), false);
	tecDetails->set_zoomed_plot(false);
	TEST(tecDetails->zoomed_plot(), false);
	tecDetails->set_show_sac(false);
	TEST(tecDetails->show_sac(), false);
	tecDetails->set_display_unused_tanks(false);
	TEST(tecDetails->display_unused_tanks(), false);
	tecDetails->set_show_average_depth(false);
	TEST(tecDetails->show_average_depth(), false);
	tecDetails->set_show_pictures_in_profile(false);
	TEST(tecDetails->show_pictures_in_profile(), false);

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

	auto location = pref->location_settings;
	location->setTimeThreshold(10);
	location->setDistanceThreshold(20);

	TEST(location->timeThreshold(), 10);
	TEST(location->distanceThreshold(), 20);

	location->setTimeThreshold(30);
	location->setDistanceThreshold(40);

	TEST(location->timeThreshold(), 30);
	TEST(location->distanceThreshold(), 40);
}

QTEST_MAIN(TestPreferences)
