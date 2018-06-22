// SPDX-License-Identifier: GPL-2.0
#include "testpreferences.h"

#include "core/subsurface-qt/SettingsObjectWrapper.h"

#include <QtTest>
#include <QDate>

#define TEST(METHOD, VALUE) \
QCOMPARE(METHOD, VALUE); \
pref->sync(); \
pref->load(); \
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

	auto cloud = pref->cloud_storage;

	cloud->setBaseUrl("test_one");
	TEST(cloud->baseUrl(), QStringLiteral("test_one"));
	cloud->setBaseUrl("test_two");
	TEST(cloud->baseUrl(), QStringLiteral("test_two"));

	cloud->setEmail("tomaz@subsurface.com");
	TEST(cloud->email(), QStringLiteral("tomaz@subsurface.com"));
	cloud->setEmail("tomaz@gmail.com");
	TEST(cloud->email(), QStringLiteral("tomaz@gmail.com"));

	cloud->setGitLocalOnly(true);
	TEST(cloud->gitLocalOnly(), true);
	cloud->setGitLocalOnly(false);
	TEST(cloud->gitLocalOnly(), false);

	// Why there's new password and password on the prefs?
	cloud->setNewPassword("ABCD");
	TEST(cloud->newPassword(), QStringLiteral("ABCD"));
	cloud->setNewPassword("ABCDE");
	TEST(cloud->newPassword(), QStringLiteral("ABCDE"));

	cloud->setPassword("ABCDE");
	TEST(cloud->password(), QStringLiteral("ABCDE"));
	cloud->setPassword("ABCABC");
	TEST(cloud->password(), QStringLiteral("ABCABC"));

	cloud->setSavePasswordLocal(true);
	TEST(cloud->savePasswordLocal(), true);
	cloud->setSavePasswordLocal(false);
	TEST(cloud->savePasswordLocal(), false);

	cloud->setSaveUserIdLocal(1);
	TEST(cloud->saveUserIdLocal(), true);
	cloud->setSaveUserIdLocal(0);
	TEST(cloud->saveUserIdLocal(), false);

	cloud->setUserId("Tomaz");
	TEST(cloud->userId(), QStringLiteral("Tomaz"));
	cloud->setUserId("Zamot");
	TEST(cloud->userId(), QStringLiteral("Zamot"));

	cloud->setVerificationStatus(0);
	TEST(cloud->verificationStatus(), (short)0);
	cloud->setVerificationStatus(1);
	TEST(cloud->verificationStatus(), (short)1);

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

	auto fb = pref->facebook;
	fb->setAccessToken("rand-access-token");
	fb->setUserId("tomaz-user-id");
	fb->setAlbumId("album-id");

	TEST(fb->accessToken(),QStringLiteral("rand-access-token"));
	TEST(fb->userId(),     QStringLiteral("tomaz-user-id"));
	TEST(fb->albumId(),    QStringLiteral("album-id"));

	fb->setAccessToken("rand-access-token-2");
	fb->setUserId("tomaz-user-id-2");
	fb->setAlbumId("album-id-2");

	TEST(fb->accessToken(),QStringLiteral("rand-access-token-2"));
	TEST(fb->userId(),     QStringLiteral("tomaz-user-id-2"));
	TEST(fb->albumId(),    QStringLiteral("album-id-2"));

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

	auto proxy = pref->proxy;
	proxy->setType(2);
	proxy->setPort(80);
	proxy->setAuth(true);
	proxy->setHost("localhost");
	proxy->setUser("unknown");
	proxy->setPass("secret");

	TEST(proxy->type(),2);
	TEST(proxy->port(),80);
	TEST(proxy->auth(),true);
	TEST(proxy->host(),QStringLiteral("localhost"));
	TEST(proxy->user(),QStringLiteral("unknown"));
	TEST(proxy->pass(),QStringLiteral("secret"));

	proxy->setType(3);
	proxy->setPort(8080);
	proxy->setAuth(false);
	proxy->setHost("127.0.0.1");
	proxy->setUser("unknown_1");
	proxy->setPass("secret_1");

	TEST(proxy->type(),3);
	TEST(proxy->port(),8080);
	TEST(proxy->auth(),false);
	TEST(proxy->host(),QStringLiteral("127.0.0.1"));
	TEST(proxy->user(),QStringLiteral("unknown_1"));
	TEST(proxy->pass(),QStringLiteral("secret_1"));

	auto planner = pref->planner_settings;
	planner->setLastStop(true);
	planner->setVerbatimPlan(true);
	planner->setDisplayRuntime(true);
	planner->setDisplayDuration(true);
	planner->setDisplayTransitions(true);
	planner->setDoo2Breaks(true);
	planner->setDropStoneMode(true);
	planner->setSafetyStop(true);
	planner->setSwitchAtRequiredStop(true);

	planner->setAscRate75(1);
	planner->setAscRate50(2);
	planner->setAscRateStops(3);
	planner->setAscRateLast6m(4);
	planner->setDescRate(5);
	planner->setBottomPo2(6);
	planner->setDecoPo2(7);
	planner->setBestmixEnd(8);
	planner->setReserveGas(9);
	planner->setMinSwitchDuration(10);
	planner->setBottomSac(11);
	planner->setDecoSac(12);

	planner->setDecoMode(BUEHLMANN);

	TEST(planner->lastStop(),true);
	TEST(planner->verbatimPlan(),true);
	TEST(planner->displayRuntime(),true);
	TEST(planner->displayDuration(),true);
	TEST(planner->displayTransitions(),true);
	TEST(planner->doo2Breaks(),true);
	TEST(planner->dropStoneMode(),true);
	TEST(planner->safetyStop(),true);
	TEST(planner->switchAtRequiredStop(),true);

	TEST(planner->ascRate75(),1);
	TEST(planner->ascRate50(),2);
	TEST(planner->ascRateStops(),3);
	TEST(planner->ascRateLast6m(),4);
	TEST(planner->descRate(),5);
	TEST(planner->bottomPo2(),6);
	TEST(planner->decoPo2(),7);
	TEST(planner->bestmixEnd(),8);
	TEST(planner->reserveGas(),9);
	TEST(planner->minSwitchDuration(),10);
	TEST(planner->bottomSac(),11);
	TEST(planner->decoSac(),12);

	TEST(planner->decoMode(),BUEHLMANN);

	planner->setLastStop(false);
	planner->setVerbatimPlan(false);
	planner->setDisplayRuntime(false);
	planner->setDisplayDuration(false);
	planner->setDisplayTransitions(false);
	planner->setDoo2Breaks(false);
	planner->setDropStoneMode(false);
	planner->setSafetyStop(false);
	planner->setSwitchAtRequiredStop(false);

	planner->setAscRate75(11);
	planner->setAscRate50(12);
	planner->setAscRateStops(13);
	planner->setAscRateLast6m(14);
	planner->setDescRate(15);
	planner->setBottomPo2(16);
	planner->setDecoPo2(17);
	planner->setBestmixEnd(18);
	planner->setReserveGas(19);
	planner->setMinSwitchDuration(110);
	planner->setBottomSac(111);
	planner->setDecoSac(112);

	planner->setDecoMode(RECREATIONAL);

	TEST(planner->lastStop(),false);
	TEST(planner->verbatimPlan(),false);
	TEST(planner->displayRuntime(),false);
	TEST(planner->displayDuration(),false);
	TEST(planner->displayTransitions(),false);
	TEST(planner->doo2Breaks(),false);
	TEST(planner->dropStoneMode(),false);
	TEST(planner->safetyStop(),false);
	TEST(planner->switchAtRequiredStop(),false);

	TEST(planner->ascRate75(),11);
	TEST(planner->ascRate50(),12);
	TEST(planner->ascRateStops(),13);
	TEST(planner->ascRateLast6m(),14);
	TEST(planner->descRate(),15);
	TEST(planner->bottomPo2(),16);
	TEST(planner->decoPo2(),17);
	TEST(planner->bestmixEnd(),18);
	TEST(planner->reserveGas(),19);
	TEST(planner->minSwitchDuration(),110);
	TEST(planner->bottomSac(),111);
	TEST(planner->decoSac(),112);

	TEST(planner->decoMode(),RECREATIONAL);

	auto units = pref->unit_settings;
	units->setLength(units::METERS);
	units->setPressure(units::BAR);
	units->setVolume(units::LITER);
	units->setTemperature(units::CELSIUS);
	units->setWeight(units::KG);
	units->setVerticalSpeedTime(units::SECONDS);
	units->setUnitSystem(QStringLiteral("metric"));
	units->setCoordinatesTraditional(false);

	TEST(units->length(),0);
	TEST(units->pressure(),0);
	TEST(units->volume(),0);
	TEST(units->temperature(),0);
	TEST(units->weight(),0);
	TEST(units->verticalSpeedTime(),0);
	TEST(units->unitSystem(),QStringLiteral("metric"));
	TEST(units->coordinatesTraditional(),false);

	units->setLength(units::FEET);
	units->setPressure(units::PSI);
	units->setVolume(units::CUFT);
	units->setTemperature(units::FAHRENHEIT);
	units->setWeight(units::LBS);
	units->setVerticalSpeedTime(units::MINUTES);
	units->setUnitSystem(QStringLiteral("fake-metric-system"));
	units->setCoordinatesTraditional(true);

	TEST(units->length(),1);
	TEST(units->pressure(),1);
	TEST(units->volume(),1);
	TEST(units->temperature(),1);
	TEST(units->weight(),1);
	TEST(units->verticalSpeedTime(),1);
	TEST(units->unitSystem(),QStringLiteral("personalized"));
	TEST(units->coordinatesTraditional(),true);

	auto general = pref->general_settings;
	general->setDefaultFilename       ("filename");
	general->setDefaultCylinder       ("cylinder_2");
	//TODOl: Change this to a enum. 	// This is 'undefined', it will need to figure out later between no_file or use_deault file.
	general->setDefaultFileBehavior   (0);
	general->setDefaultSetPoint       (0);
	general->setO2Consumption         (0);
	general->setPscrRatio             (0);
	general->setUseDefaultFile        (true);

	TEST(general->defaultFilename(), QStringLiteral("filename"));
	TEST(general->defaultCylinder(), QStringLiteral("cylinder_2"));
	TEST(general->defaultFileBehavior(), (short) LOCAL_DEFAULT_FILE); // since we have a default file, here it returns
	TEST(general->defaultSetPoint(), 0);
	TEST(general->o2Consumption(), 0);
	TEST(general->pscrRatio(), 0);
	TEST(general->useDefaultFile(), true);

	general->setDefaultFilename       ("no_file_name");
	general->setDefaultCylinder       ("cylinder_1");
	//TODOl: Change this to a enum.
	general->setDefaultFileBehavior   (CLOUD_DEFAULT_FILE);

	general->setDefaultSetPoint       (1);
	general->setO2Consumption         (1);
	general->setPscrRatio             (1);
	general->setUseDefaultFile        (false);

	TEST(general->defaultFilename(), QStringLiteral("no_file_name"));
	TEST(general->defaultCylinder(), QStringLiteral("cylinder_1"));
	TEST(general->defaultFileBehavior(), (short) CLOUD_DEFAULT_FILE);
	TEST(general->defaultSetPoint(), 1);
	TEST(general->o2Consumption(), 1);
	TEST(general->pscrRatio(), 1);
	TEST(general->useDefaultFile(), false);

	auto display = pref->display_settings;
	display->setDivelistFont("comic");
	display->setFontSize(10.0);
	display->setDisplayInvalidDives(true);

	TEST(display->divelistFont(),QStringLiteral("comic"));
	TEST(display->fontSize(), 10.0);
	TEST(display->displayInvalidDives(), true);

	display->setDivelistFont("helvetica");
	display->setFontSize(14.0);
	display->setDisplayInvalidDives(false);

	TEST(display->divelistFont(),QStringLiteral("helvetica"));
	TEST(display->fontSize(), 14.0);
	TEST(display->displayInvalidDives(), false);

	auto language = pref->language_settings;
	language->setLangLocale         ("en_US");
	language->setLanguage           ("en");
	language->setTimeFormat         ("hh:mm");
	language->setDateFormat         ("dd/mm/yy");
	language->setDateFormatShort    ("dd/mm");
	language->setTimeFormatOverride (false);
	language->setDateFormatOverride (false);
	language->setUseSystemLanguage  (false);

	TEST(language->langLocale(), QStringLiteral("en_US"));
	TEST(language->language(), QStringLiteral("en"));
	TEST(language->timeFormat(), QStringLiteral("hh:mm"));
	TEST(language->dateFormat(), QStringLiteral("dd/mm/yy"));
	TEST(language->dateFormatShort(), QStringLiteral("dd/mm"));
	TEST(language->timeFormatOverride(), false);
	TEST(language->dateFormatOverride(), false);
	TEST(language->useSystemLanguage(), false);

	language->setLangLocale         ("en_EN");
	language->setLanguage           ("br");
	language->setTimeFormat         ("mm:hh");
	language->setDateFormat         ("yy/mm/dd");
	language->setDateFormatShort    ("dd/yy");
	language->setTimeFormatOverride (true);
	language->setDateFormatOverride (true);
	language->setUseSystemLanguage  (true);

	TEST(language->langLocale(), QStringLiteral("en_EN"));
	TEST(language->language(), QStringLiteral("br"));
	TEST(language->timeFormat(), QStringLiteral("mm:hh"));
	TEST(language->dateFormat(), QStringLiteral("yy/mm/dd"));
	TEST(language->dateFormatShort(), QStringLiteral("dd/yy"));
	TEST(language->timeFormatOverride(),true);
	TEST(language->dateFormatOverride(),true);
	TEST(language->useSystemLanguage(), true);

	pref->animation_settings->setAnimationSpeed(20);
	TEST(pref->animation_settings->animationSpeed(), 20);
	pref->animation_settings->setAnimationSpeed(30);
	TEST(pref->animation_settings->animationSpeed(), 30);

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

	auto dc = pref->dive_computer_settings;
	dc->setDevice("TomazComputer");
	TEST(dc->device(), QStringLiteral("TomazComputer"));
	dc->setDevice("Deepwater");
	TEST(dc->device(), QStringLiteral("Deepwater"));

	dc->setDownloadMode(0);
	TEST(dc->downloadMode(), 0);
	dc->setDownloadMode(1);
	TEST(dc->downloadMode(), 1);

	dc->setProduct("Thingy1");
	TEST(dc->product(), QStringLiteral("Thingy1"));
	dc->setProduct("Thingy2");
	TEST(dc->product(), QStringLiteral("Thingy2"));

	dc->setVendor("Sharewater");
	TEST(dc->vendor(), QStringLiteral("Sharewater"));
	dc->setVendor("OSTS");
	TEST(dc->vendor(), QStringLiteral("OSTS"));
}

QTEST_MAIN(TestPreferences)
