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
}

QTEST_MAIN(TestPreferences)
