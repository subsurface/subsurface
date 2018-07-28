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

	auto planner = pref->planner_settings;
	planner->setLastStop(true);
	planner->setVerbatimPlan(true);
	planner->setDisplayRuntime(true);
	planner->setDisplayDuration(true);
	planner->setDisplayTransitions(true);
	planner->setDoo2breaks(true);
	planner->setDropStoneMode(true);
	planner->setSafetyStop(true);
	planner->setSwitchAtRequiredStop(true);

	planner->setAscrate75(1);
	planner->setAscrate50(2);
	planner->setAscratestops(3);
	planner->setAscratelast6m(4);
	planner->setDescrate(5);
	planner->setBottompo2(6);
	planner->setDecopo2(7);
	planner->setBestmixend(8);
	planner->setReserveGas(9);
	planner->setMinSwitchDuration(10);
	planner->setBottomSac(11);
	planner->setDecoSac(12);

	planner->setDecoMode(BUEHLMANN);

	TEST(planner->lastStop(), true);
	TEST(planner->verbatimPlan(), true);
	TEST(planner->displayRuntime(), true);
	TEST(planner->displayDuration(), true);
	TEST(planner->displayTransitions(), true);
	TEST(planner->doo2breaks(), true);
	TEST(planner->dropStoneMode(), true);
	TEST(planner->safetyStop(), true);
	TEST(planner->switchAtRequiredStop(), true);

	TEST(planner->ascrate75(), 1);
	TEST(planner->ascrate50(), 2);
	TEST(planner->ascratestops(), 3);
	TEST(planner->ascratelast6m(), 4);
	TEST(planner->descrate(), 5);
	TEST(planner->bottompo2(), 6);
	TEST(planner->decopo2(), 7);
	TEST(planner->bestmixend(), 8);
	TEST(planner->reserveGas(), 9);
	TEST(planner->minSwitchDuration(), 10);
	TEST(planner->bottomSac(), 11);
	TEST(planner->decoSac(), 12);

	TEST(planner->decoMode(), BUEHLMANN);

	planner->setLastStop(false);
	planner->setVerbatimPlan(false);
	planner->setDisplayRuntime(false);
	planner->setDisplayDuration(false);
	planner->setDisplayTransitions(false);
	planner->setDoo2breaks(false);
	planner->setDropStoneMode(false);
	planner->setSafetyStop(false);
	planner->setSwitchAtRequiredStop(false);

	planner->setAscrate75(11);
	planner->setAscrate50(12);
	planner->setAscratestops(13);
	planner->setAscratelast6m(14);
	planner->setDescrate(15);
	planner->setBottompo2(16);
	planner->setDecopo2(17);
	planner->setBestmixend(18);
	planner->setReserveGas(19);
	planner->setMinSwitchDuration(110);
	planner->setBottomSac(111);
	planner->setDecoSac(112);

	planner->setDecoMode(RECREATIONAL);

	TEST(planner->lastStop(), false);
	TEST(planner->verbatimPlan(), false);
	TEST(planner->displayRuntime(), false);
	TEST(planner->displayDuration(), false);
	TEST(planner->displayTransitions(), false);
	TEST(planner->doo2breaks(), false);
	TEST(planner->dropStoneMode(), false);
	TEST(planner->safetyStop(), false);
	TEST(planner->switchAtRequiredStop(), false);

	TEST(planner->ascrate75(), 11);
	TEST(planner->ascrate50(), 12);
	TEST(planner->ascratestops(), 13);
	TEST(planner->ascratelast6m(), 14);
	TEST(planner->descrate(), 15);
	TEST(planner->bottompo2(), 16);
	TEST(planner->decopo2(), 17);
	TEST(planner->bestmixend(), 18);
	TEST(planner->reserveGas(), 19);
	TEST(planner->minSwitchDuration(), 110);
	TEST(planner->bottomSac(), 111);
	TEST(planner->decoSac(), 112);

	TEST(planner->decoMode(), RECREATIONAL);

	auto units = pref->unit_settings;
	units->setLength(0);
	units->setPressure(0);
	units->setVolume(0);
	units->setTemperature(0);
	units->setWeight(0);
	units->setVerticalSpeedTime(0);
	units->setUnitSystem(QStringLiteral("metric"));
	units->setCoordinatesTraditional(false);

	TEST(units->length(), 0);
	TEST(units->pressure(), 0);
	TEST(units->volume(), 0);
	TEST(units->temperature(), 0);
	TEST(units->weight(), 0);
	TEST(units->verticalSpeedTime(), 0);
	TEST(units->unitSystem(), QStringLiteral("metric"));
	TEST(units->coordinatesTraditional(), false);

	units->setLength(1);
	units->setPressure(1);
	units->setVolume(1);
	units->setTemperature(1);
	units->setWeight(1);
	units->setVerticalSpeedTime(1);
	units->setUnitSystem(QStringLiteral("fake-metric-system"));
	units->setCoordinatesTraditional(true);

	TEST(units->length(), 1);
	TEST(units->pressure(), 1);
	TEST(units->volume(), 1);
	TEST(units->temperature(), 1);
	TEST(units->weight(), 1);
	TEST(units->verticalSpeedTime(), 1);
	TEST(units->unitSystem(), QStringLiteral("personalized"));
	TEST(units->coordinatesTraditional(), true);

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
