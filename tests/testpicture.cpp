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
	struct picture *pic;
	verbose = 1;

	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test44.xml"), 0);
	dive = get_dive(0);
	QVERIFY(dive != NULL);
	pic = dive->picture_list;
	// So far no picture in dive
	QVERIFY(pic == NULL);

	dive_create_picture(dive, SUBSURFACE_TEST_DATA "/wreck.jpg", 0, false);
	pic = dive->picture_list;
	// Now there is a picture
	QVERIFY(pic != NULL);
	// Appearing at time 21:01
	QVERIFY(pic->offset.seconds == 1261);
	QVERIFY(pic->latitude.udeg == 47934500);
	QVERIFY(pic->longitude.udeg == 11334500);

	QVERIFY(pic->hash == NULL);
	learnHash(pic, hashFile(localFilePath(pic->filename)));
	QCOMPARE(pic->hash, "929ad9499b7ae7a9e39ef63eb6c239604ac2adfa");

}


QTEST_GUILESS_MAIN(TestPicture)
