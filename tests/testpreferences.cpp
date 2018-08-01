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

	auto tecDetails = pref->techDetails;
	tecDetails->setModpO2(0.2);
	TEST(tecDetails->modpO2(), 0.2);
	tecDetails->setModpO2(1.0);
	TEST(tecDetails->modpO2(), 1.0);

	tecDetails->setGflow(2);
	TEST(tecDetails->gflow(), 2);
	tecDetails->setGflow(3);
	TEST(tecDetails->gflow(), 3);

	tecDetails->setGfhigh(4);
	TEST(tecDetails->gfhigh(), 4);
	tecDetails->setGfhigh(5);
	TEST(tecDetails->gfhigh(), 5);

	tecDetails->setVpmbConservatism(5);
	TEST(tecDetails->vpmbConservatism(), (short)5);
	tecDetails->setVpmbConservatism(6);
	TEST(tecDetails->vpmbConservatism(), (short)6);

	tecDetails->setEad(true);
	TEST(tecDetails->ead(), true);
	tecDetails->setMod(true);
	TEST(tecDetails->mod(), true);
	tecDetails->setDCceiling(true);
	TEST(tecDetails->dcceiling(), true);
	tecDetails->setRedceiling(true);
	TEST(tecDetails->redceiling(), true);
	tecDetails->setCalcceiling(true);
	TEST(tecDetails->calcceiling(), true);
	tecDetails->setCalcceiling3m(true);
	TEST(tecDetails->calcceiling3m(), true);
	tecDetails->setCalcalltissues(true);
	TEST(tecDetails->calcalltissues(), true);
	tecDetails->setCalcndltts(true);
	TEST(tecDetails->calcndltts(), true);
	tecDetails->setBuehlmann(true);
	TEST(tecDetails->buehlmann(), true);
	tecDetails->setHRgraph(true);
	TEST(tecDetails->hrgraph(), true);
	tecDetails->setTankBar(true);
	TEST(tecDetails->tankBar(), true);
	tecDetails->setPercentageGraph(true);
	TEST(tecDetails->percentageGraph(), true);
	tecDetails->setRulerGraph(true);
	TEST(tecDetails->rulerGraph(), true);
	tecDetails->setShowCCRSetpoint(true);
	TEST(tecDetails->showCCRSetpoint(), true);
	tecDetails->setShowCCRSensors(true);
	TEST(tecDetails->showCCRSensors(), true);
	tecDetails->setZoomedPlot(true);
	TEST(tecDetails->zoomedPlot(), true);
	tecDetails->setShowSac(true);
	TEST(tecDetails->showSac(), true);
	tecDetails->setDisplayUnusedTanks(true);
	TEST(tecDetails->displayUnusedTanks(), true);
	tecDetails->setShowAverageDepth(true);
	TEST(tecDetails->showAverageDepth(), true);
	tecDetails->setShowPicturesInProfile(true);
	TEST(tecDetails->showPicturesInProfile(), true);

	tecDetails->setEad(false);
	TEST(tecDetails->ead(), false);
	tecDetails->setMod(false);
	TEST(tecDetails->mod(), false);
	tecDetails->setDCceiling(false);
	TEST(tecDetails->dcceiling(), false);
	tecDetails->setRedceiling(false);
	TEST(tecDetails->redceiling(), false);
	tecDetails->setCalcceiling(false);
	TEST(tecDetails->calcceiling(), false);
	tecDetails->setCalcceiling3m(false);
	TEST(tecDetails->calcceiling3m(), false);
	tecDetails->setCalcalltissues(false);
	TEST(tecDetails->calcalltissues(), false);
	tecDetails->setCalcndltts(false);
	TEST(tecDetails->calcndltts(), false);
	tecDetails->setBuehlmann(false);
	TEST(tecDetails->buehlmann(), false);
	tecDetails->setHRgraph(false);
	TEST(tecDetails->hrgraph(), false);
	tecDetails->setTankBar(false);
	TEST(tecDetails->tankBar(), false);
	tecDetails->setPercentageGraph(false);
	TEST(tecDetails->percentageGraph(), false);
	tecDetails->setRulerGraph(false);
	TEST(tecDetails->rulerGraph(), false);
	tecDetails->setShowCCRSetpoint(false);
	TEST(tecDetails->showCCRSetpoint(), false);
	tecDetails->setShowCCRSensors(false);
	TEST(tecDetails->showCCRSensors(), false);
	tecDetails->setZoomedPlot(false);
	TEST(tecDetails->zoomedPlot(), false);
	tecDetails->setShowSac(false);
	TEST(tecDetails->showSac(), false);
	tecDetails->setDisplayUnusedTanks(false);
	TEST(tecDetails->displayUnusedTanks(), false);
	tecDetails->setShowAverageDepth(false);
	TEST(tecDetails->showAverageDepth(), false);
	tecDetails->setShowPicturesInProfile(false);
	TEST(tecDetails->showPicturesInProfile(), false);

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

	auto planner = qPrefDivePlanner::instance();
	depth_t x;

	planner->set_last_stop(true);
	planner->set_verbatim_plan(true);
	planner->set_display_runtime(true);
	planner->set_display_duration(true);
	planner->set_display_transitions(true);
	planner->set_doo2breaks(true);
	planner->set_drop_stone_mode(true);
	planner->set_safetystop(true);
	planner->set_switch_at_req_stop(true);

	planner->set_ascrate75(1);
	planner->set_ascrate50(2);
	planner->set_ascratestops(3);
	planner->set_ascratelast6m(4);
	planner->set_descrate(5);
	planner->set_bottompo2(6);
	planner->set_decopo2(7);
	x.mm = 8;
	planner->set_bestmixend(x);
	planner->set_reserve_gas(9);
	planner->set_min_switch_duration(10);
	planner->set_bottomsac(11);
	planner->set_decosac(12);

	planner->set_planner_deco_mode(BUEHLMANN);

	TEST(planner->last_stop(), true);
	TEST(planner->verbatim_plan(), true);
	TEST(planner->display_runtime(), true);
	TEST(planner->display_duration(), true);
	TEST(planner->display_transitions(), true);
	TEST(planner->doo2breaks(), true);
	TEST(planner->drop_stone_mode(), true);
	TEST(planner->safetystop(), true);
	TEST(planner->switch_at_req_stop(), true);

	TEST(planner->ascrate75(), 1);
	TEST(planner->ascrate50(), 2);
	TEST(planner->ascratestops(), 3);
	TEST(planner->ascratelast6m(), 4);
	TEST(planner->descrate(), 5);
	TEST(planner->bottompo2(), 6);
	TEST(planner->decopo2(), 7);
	TEST(planner->bestmixend().mm, 8);
	TEST(planner->reserve_gas(), 9);
	TEST(planner->min_switch_duration(), 10);
	TEST(planner->bottomsac(), 11);
	TEST(planner->decosac(), 12);

	TEST(planner->planner_deco_mode(), BUEHLMANN);

	planner->set_last_stop(false);
	planner->set_verbatim_plan(false);
	planner->set_display_runtime(false);
	planner->set_display_duration(false);
	planner->set_display_transitions(false);
	planner->set_doo2breaks(false);
	planner->set_drop_stone_mode(false);
	planner->set_safetystop(false);
	planner->set_switch_at_req_stop(false);

	planner->set_ascrate75(11);
	planner->set_ascrate50(12);
	planner->set_ascratestops(13);
	planner->set_ascratelast6m(14);
	planner->set_descrate(15);
	planner->set_bottompo2(16);
	planner->set_decopo2(17);
	x.mm = 18;
	planner->set_bestmixend(x);
	planner->set_reserve_gas(19);
	planner->set_min_switch_duration(110);
	planner->set_bottomsac(111);
	planner->set_decosac(112);

	planner->set_planner_deco_mode(RECREATIONAL);

	TEST(planner->last_stop(), false);
	TEST(planner->verbatim_plan(), false);
	TEST(planner->display_runtime(), false);
	TEST(planner->display_duration(), false);
	TEST(planner->display_transitions(), false);
	TEST(planner->doo2breaks(), false);
	TEST(planner->drop_stone_mode(), false);
	TEST(planner->safetystop(), false);
	TEST(planner->switch_at_req_stop(), false);

	TEST(planner->ascrate75(), 11);
	TEST(planner->ascrate50(), 12);
	TEST(planner->ascratestops(), 13);
	TEST(planner->ascratelast6m(), 14);
	TEST(planner->descrate(), 15);
	TEST(planner->bottompo2(), 16);
	TEST(planner->decopo2(), 17);
	TEST(planner->bestmixend().mm, 18);
	TEST(planner->reserve_gas(), 19);
	TEST(planner->min_switch_duration(), 110);
	TEST(planner->bottomsac(), 111);
	TEST(planner->decosac(), 112);

	TEST(planner->planner_deco_mode(), RECREATIONAL);

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

	auto update = pref->update_manager_settings;
	QDate date = QDate::currentDate();

	update->setDontCheckForUpdates(true);
	update->setLastVersionUsed("tomaz-1");
	update->setNextCheck(date);

	TEST(update->dontCheckForUpdates(), true);
	TEST(update->lastVersionUsed(), QStringLiteral("tomaz-1"));
	TEST(update->nextCheck(), date);

	date = date.addDays(3);
	update->setDontCheckForUpdates(false);
	update->setLastVersionUsed("tomaz-2");
	update->setNextCheck(date);

	TEST(update->dontCheckForUpdates(), false);
	TEST(update->lastVersionUsed(), QStringLiteral("tomaz-2"));
	TEST(update->nextCheck(), date);
}

QTEST_MAIN(TestPreferences)
