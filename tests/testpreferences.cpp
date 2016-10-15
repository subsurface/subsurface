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
	pp->setShowPn2(1);
	pp->setShowPhe(2);
	pp->setShowPo2(3);
	pp->setPo2Threshold(1.0);
	pp->setPn2Threshold(2.0);
	pp->setPheThreshold(3.0);

	TEST(pp->showPn2(), (short) 1);
	TEST(pp->showPhe(), (short) 2);
	TEST(pp->showPo2(), (short) 3);
	TEST(pp->pn2Threshold(), 1.0);
	TEST(pp->pheThreshold(), 2.0);
	TEST(pp->po2Threshold(), 3.0);

	pp->setShowPn2(4);
	pp->setShowPhe(5);
	pp->setShowPo2(6);
	pp->setPo2Threshold(4.0);
	pp->setPn2Threshold(5.0);
	pp->setPheThreshold(6.0);

	TEST(pp->showPn2(), (short) 4);
	TEST(pp->showPhe(), (short) 5);
	TEST(pp->showPo2(), (short) 6);
	TEST(pp->pn2Threshold(), 4.0);
	TEST(pp->pheThreshold(), 5.0);
	TEST(pp->po2Threshold(), 6.0);

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
}

QTEST_MAIN(TestPreferences)
