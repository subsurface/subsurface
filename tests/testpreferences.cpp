#include "testpreferences.h"

#include "core/subsurface-qt/SettingsObjectWrapper.h"

#include <QtTest>

#define TEST(METHOD, VALUE) \
QCOMPARE(METHOD, VALUE); \
pref->sync(); \
pref->load(); \
QCOMPARE(METHOD, VALUE);


void TestPreferences::testPreferences()
{
	auto pref = SettingsObjectWrapper::instance();
	pref->load();

	pref->animation_settings->setAnimationSpeed(20);
	TEST(pref->animation_settings->animationSpeed(), 20);
	pref->animation_settings->setAnimationSpeed(30);
	TEST(pref->animation_settings->animationSpeed(), 30);

	auto cloud = pref->cloud_storage;
	cloud->setBackgroundSync(true);
	TEST(cloud->backgroundSync(), true);
	cloud->setBackgroundSync(false);
	TEST(cloud->backgroundSync(), false);

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

	// Why this is short and not bool?
	cloud->setSaveUserIdLocal(1);
	TEST(cloud->saveUserIdLocal(), (short)1);
	cloud->setSaveUserIdLocal(0);
	TEST(cloud->saveUserIdLocal(), (short)0);

	cloud->setUserId("Tomaz");
	TEST(cloud->userId(), QStringLiteral("Tomaz"));
	cloud->setUserId("Zamot");
	TEST(cloud->userId(), QStringLiteral("Zamot"));

	cloud->setVerificationStatus(0);
	TEST(cloud->verificationStatus(), (short)0);
	cloud->setVerificationStatus(1);
	TEST(cloud->verificationStatus(), (short)1);

	auto dc = pref->dive_computer_settings;
	dc->setDevice("TomazComputer");
	TEST(dc->dc_device(), QStringLiteral("TomazComputer"));
	dc->setDevice("Deepwater");
	TEST(dc->dc_device(), QStringLiteral("Deepwater"));

	dc->setDownloadMode(0);
	TEST(dc->downloadMode(), 0);
	dc->setDownloadMode(1);
	TEST(dc->downloadMode(), 1);

	dc->setProduct("Thingy1");
	TEST(dc->dc_product(), QStringLiteral("Thingy1"));
	dc->setProduct("Thingy2");
	TEST(dc->dc_product(), QStringLiteral("Thingy2"));

	dc->setVendor("Sharewater");
	TEST(dc->dc_vendor(), QStringLiteral("Sharewater"));
	dc->setVendor("OSTS");
	TEST(dc->dc_vendor(), QStringLiteral("OSTS"));

	auto tecDetails = pref->techDetails;
	tecDetails->setModp02(0.2);
	TEST(tecDetails->modp02(), 0.2);
	tecDetails->setModp02(1.0);
	TEST(tecDetails->modp02(), 1.0);

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
	tecDetails->setGfLowAtMaxDepth(true);
	TEST(tecDetails->gfLowAtMaxDepth(), true);
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
	tecDetails->setGfLowAtMaxDepth(false);
	TEST(tecDetails->gfLowAtMaxDepth(), false);
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
	pp->setPo2Threshold(1.0);
	pp->setPn2Threshold(2.0);
	pp->setPheThreshold(3.0);

	TEST(pp->showPn2(), (short) false);
	TEST(pp->showPhe(), (short) false);
	TEST(pp->showPo2(), (short) false);
	TEST(pp->pn2Threshold(), 2.0);
	TEST(pp->pheThreshold(), 3.0);
	TEST(pp->po2Threshold(), 1.0);

	pp->setShowPn2(true);
	pp->setShowPhe(true);
	pp->setShowPo2(true);
	pp->setPo2Threshold(4.0);
	pp->setPn2Threshold(5.0);
	pp->setPheThreshold(6.0);

	TEST(pp->showPn2(), (short) true);
	TEST(pp->showPhe(), (short) true);
	TEST(pp->showPo2(), (short) true);
	TEST(pp->pn2Threshold(), 5.0);
	TEST(pp->pheThreshold(), 6.0);
	TEST(pp->po2Threshold(), 4.0);

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
	geo->setEnableGeocoding(true);
	geo->setParseDiveWithoutGps(true);
	geo->setTagExistingDives(true);

	TEST(geo->enableGeocoding(),true);
	TEST(geo->parseDiveWithoutGps(),true);
	TEST(geo->tagExistingDives(),true);

	geo->setFirstTaxonomyCategory(TC_NONE);
	geo->setSecondTaxonomyCategory(TC_OCEAN);
	geo->setThirdTaxonomyCategory(TC_COUNTRY);

	TEST(geo->firstTaxonomyCategory(), TC_NONE);
	TEST(geo->secondTaxonomyCategory(), TC_OCEAN);
	TEST(geo->thirdTaxonomyCategory(), TC_COUNTRY);

	geo->setEnableGeocoding(false);
	geo->setParseDiveWithoutGps(false);
	geo->setTagExistingDives(false);

	TEST(geo->enableGeocoding(),false);
	TEST(geo->parseDiveWithoutGps(),false);
	TEST(geo->tagExistingDives(),false);

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

	TEST(planner->lastStop(),true);
	TEST(planner->verbatimPlan(),true);
	TEST(planner->displayRuntime(),true);
	TEST(planner->displayDuration(),true);
	TEST(planner->displayTransitions(),true);
	TEST(planner->doo2breaks(),true);
	TEST(planner->dropStoneMode(),true);
	TEST(planner->safetyStop(),true);
	TEST(planner->switchAtRequiredStop(),true);

	TEST(planner->ascrate75(),1);
	TEST(planner->ascrate50(),2);
	TEST(planner->ascratestops(),3);
	TEST(planner->ascratelast6m(),4);
	TEST(planner->descrate(),5);
	TEST(planner->bottompo2(),6);
	TEST(planner->decopo2(),7);
	TEST(planner->bestmixend(),8);
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

	TEST(planner->lastStop(),false);
	TEST(planner->verbatimPlan(),false);
	TEST(planner->displayRuntime(),false);
	TEST(planner->displayDuration(),false);
	TEST(planner->displayTransitions(),false);
	TEST(planner->doo2breaks(),false);
	TEST(planner->dropStoneMode(),false);
	TEST(planner->safetyStop(),false);
	TEST(planner->switchAtRequiredStop(),false);

	TEST(planner->ascrate75(),11);
	TEST(planner->ascrate50(),12);
	TEST(planner->ascratestops(),13);
	TEST(planner->ascratelast6m(),14);
	TEST(planner->descrate(),15);
	TEST(planner->bottompo2(),16);
	TEST(planner->decopo2(),17);
	TEST(planner->bestmixend(),18);
	TEST(planner->reserveGas(),19);
	TEST(planner->minSwitchDuration(),110);
	TEST(planner->bottomSac(),111);
	TEST(planner->decoSac(),112);

	TEST(planner->decoMode(),RECREATIONAL);

	auto units = pref->unit_settings;
	units->setLength(0);
	units->setPressure(0);
	units->setVolume(0);
	units->setTemperature(0);
	units->setWeight(0);
	units->setVerticalSpeedTime(0);
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

	units->setLength(1);
	units->setPressure(1);
	units->setVolume(1);
	units->setTemperature(1);
	units->setWeight(1);
	units->setVerticalSpeedTime(1);
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
	//TODOl: Change this to a enum.
	general->setDefaultFileBehavior   (0);

	general->setDefaultSetPoint       (0);
	general->setO2Consumption         (0);
	general->setPscrRatio             (0);
	general->setUseDefaultFile        (true);

	TEST(general->defaultFilename(), QStringLiteral("filename"));
	TEST(general->defaultCylinder(), QStringLiteral("cylinder_2"));
	TEST(general->defaultFileBehavior(), (short)0);
	TEST(general->defaultSetPoint(), 0);
	TEST(general->o2Consumption(), 0);
	TEST(general->pscrRatio(), 0);
	TEST(general->useDefaultFile(), true);

	general->setDefaultFilename       ("no_file_name");
	general->setDefaultCylinder       ("cylinder_1");
	//TODOl: Change this to a enum.
	general->setDefaultFileBehavior   (1);

	general->setDefaultSetPoint       (1);
	general->setO2Consumption         (1);
	general->setPscrRatio             (1);
	general->setUseDefaultFile        (false);

	TEST(general->defaultFilename(), QStringLiteral("no_file_name"));
	TEST(general->defaultCylinder(), QStringLiteral("cylinder_1"));
	TEST(general->defaultFileBehavior(), (short) 1);
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
	TEST(display->displayInvalidDives(),(short) true); //TODO: this is true / false.

	display->setDivelistFont("helvetica");
	display->setFontSize(14.0);
	display->setDisplayInvalidDives(false);

	TEST(display->divelistFont(),QStringLiteral("helvetica"));
	TEST(display->fontSize(), 14.0);
	TEST(display->displayInvalidDives(),(short) false);

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
}

QTEST_MAIN(TestPreferences)
