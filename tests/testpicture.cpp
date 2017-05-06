// SPDX-License-Identifier: GPL-2.0
#include "testpicture.h"
#include "core/dive.h"
#include "core/file.h"
#include "core/divelist.h"
#include <QString>
#include <core/qthelper.h>

void TestPicture::initTestCase()
{
	/* we need to manually tell that the resource exists, because we are using it as library. */
	Q_INIT_RESOURCE(subsurface);
}

void TestPicture::addPicture()
{
	struct dive *dive;
	struct picture *pic1, *pic2;
	verbose = 1;

	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test44.xml"), 0);
	dive = get_dive(0);
	QVERIFY(dive != NULL);
	pic1 = dive->picture_list;
	// So far no picture in dive
	QVERIFY(pic1 == NULL);

	dive_create_picture(dive, SUBSURFACE_TEST_DATA "/dives/images/wreck.jpg", 0, false);
	dive_create_picture(dive, SUBSURFACE_TEST_DATA "/dives/images/data_after_EOI.jpg", 0, false);
	pic1 = dive->picture_list;
	pic2 = pic1->next;
	// Now there are two picture2
	QVERIFY(pic1 != NULL);
	QVERIFY(pic2 != NULL);
	// 1st appearing at time 21:01
	// 2nd appearing at time 22:01
	QVERIFY(pic1->offset.seconds == 1261);
	QVERIFY(pic1->latitude.udeg == 47934500);
	QVERIFY(pic1->longitude.udeg == 11334500);
	QVERIFY(pic2->offset.seconds == 1321);

	QVERIFY(pic1->hash == NULL);
	QVERIFY(pic2->hash == NULL);
	learnHash(pic1, hashFile(localFilePath(pic1->filename)));
	learnHash(pic2, hashFile(localFilePath(pic2->filename)));
	QCOMPARE(pic1->hash, "929ad9499b7ae7a9e39ef63eb6c239604ac2adfa");
	QCOMPARE(pic2->hash, "fa8bd48f8f24017a81e1204f52300bd98b43d4a7");

}


QTEST_GUILESS_MAIN(TestPicture)
