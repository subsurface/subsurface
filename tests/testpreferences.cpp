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

	pref->cloud_storage->setBackgroundSync(true);
	TEST(pref->cloud_storage->backgroundSync(), true);
	pref->cloud_storage->setBackgroundSync(false);
	TEST(pref->cloud_storage->backgroundSync(), false);

	pref->cloud_storage->setBaseUrl("test_one");
	TEST(pref->cloud_storage->baseUrl(), QStringLiteral("test_one"));
	pref->cloud_storage->setBaseUrl("test_two");
	TEST(pref->cloud_storage->baseUrl(), QStringLiteral("test_two"));

	pref->cloud_storage->setEmail("tomaz@subsurface.com");
	TEST(pref->cloud_storage->email(), QStringLiteral("tomaz@subsurface.com"));
	pref->cloud_storage->setEmail("tomaz@gmail.com");
	TEST(pref->cloud_storage->email(), QStringLiteral("tomaz@gmail.com"));

	pref->cloud_storage->setGitLocalOnly(true);
	TEST(pref->cloud_storage->gitLocalOnly(), true);
	pref->cloud_storage->setGitLocalOnly(false);
	TEST(pref->cloud_storage->gitLocalOnly(), false);
}

QTEST_MAIN(TestPreferences)
