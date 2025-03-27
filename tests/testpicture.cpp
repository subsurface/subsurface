// SPDX-License-Identifier: GPL-2.0
#include "testpicture.h"
#include "core/device.h"
#include "core/dive.h"
#include "core/divelog.h"
#include "core/errorhelper.h"
#include "core/picture.h"
#include "core/file.h"
#include "core/pref.h"
#include <QString>
#include <core/qthelper.h>

void TestPicture::initTestCase()
{
	TestBase::initTestCase();
	prefs.cloud_base_url = default_prefs.cloud_base_url;
}

#define PIC1_NAME "/dives/images/wreck.jpg"
#define PIC2_NAME "/dives/images/data_after_EOI.jpg"
#define PIC1_HASH "929ad9499b7ae7a9e39ef63eb6c239604ac2adfa"
#define PIC2_HASH "fa8bd48f8f24017a81e1204f52300bd98b43d4a7"

void TestPicture::addPicture()
{
	verbose = 1;

	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test44.xml", &divelog), 0);
	QVERIFY(divelog.dives.size() >= 1);
	struct dive &dive = *divelog.dives[0];
	// Pictures will be added to selected dives
	dive.selected = true;
	// So far no picture in dive
	QVERIFY(dive.pictures.size() == 0);

	{
		auto [pic1, dive1] = create_picture(SUBSURFACE_TEST_DATA "/dives/images/wreck.jpg", 0, false);
		auto [pic2, dive2] = create_picture(SUBSURFACE_TEST_DATA "/dives/images/data_after_EOI.jpg", 0, false);
		QVERIFY(pic1);
		QVERIFY(pic2);
		QVERIFY(dive1 == &dive);
		QVERIFY(dive2 == &dive);

		add_picture(dive.pictures, std::move(*pic1));
		add_picture(dive.pictures, std::move(*pic2));
	}

	// Now there are two pictures
	QVERIFY(dive.pictures.size() == 2);
	const picture &pic1 = dive.pictures[0];
	const picture &pic2 = dive.pictures[1];
	// 1st appearing at time 21:01
	// 2nd appearing at time 22:01
	QVERIFY(pic1.offset.seconds == 1261);
	QVERIFY(pic1.location.lat.udeg == 47934500);
	QVERIFY(pic1.location.lon.udeg == 11334500);
	QVERIFY(pic2.offset.seconds == 1321);

	learnPictureFilename(QString::fromStdString(pic1.filename), PIC1_NAME);
	learnPictureFilename(QString::fromStdString(pic2.filename), PIC2_NAME);
	QCOMPARE(localFilePath(QString::fromStdString(pic1.filename)), QString(PIC1_NAME));
	QCOMPARE(localFilePath(QString::fromStdString(pic2.filename)), QString(PIC2_NAME));
}

QTEST_GUILESS_MAIN(TestPicture)
