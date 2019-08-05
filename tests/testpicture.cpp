// SPDX-License-Identifier: GPL-2.0
#include "testpicture.h"
#include "core/divesite.h"
#include "core/errorhelper.h"
#include "core/trip.h"
#include "core/file.h"
#include <QString>
#include <core/qthelper.h>

void TestPicture::initTestCase()
{
	/* we need to manually tell that the resource exists, because we are using it as library. */
	Q_INIT_RESOURCE(subsurface);
}

#define PIC1_NAME "/dives/images/wreck.jpg"
#define PIC2_NAME "/dives/images/data_after_EOI.jpg"
#define PIC1_HASH "929ad9499b7ae7a9e39ef63eb6c239604ac2adfa"
#define PIC2_HASH "fa8bd48f8f24017a81e1204f52300bd98b43d4a7"

void TestPicture::addPicture()
{
	struct dive *dive;
	struct picture *pic1, *pic2;
	verbose = 1;

	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test44.xml", &dive_table, &trip_table, &dive_site_table), 0);
	dive = get_dive(0);
	// Pictures will be added to selected dives
	dive->selected = true;
	QVERIFY(dive != NULL);
	pic1 = dive->picture_list;
	// So far no picture in dive
	QVERIFY(pic1 == NULL);

	create_picture(SUBSURFACE_TEST_DATA "/dives/images/wreck.jpg", 0, false);
	create_picture(SUBSURFACE_TEST_DATA "/dives/images/data_after_EOI.jpg", 0, false);
	pic1 = dive->picture_list;
	pic2 = pic1->next;
	// Now there are two picture2
	QVERIFY(pic1 != NULL);
	QVERIFY(pic2 != NULL);
	// 1st appearing at time 21:01
	// 2nd appearing at time 22:01
	QVERIFY(pic1->offset.seconds == 1261);
	QVERIFY(pic1->location.lat.udeg == 47934500);
	QVERIFY(pic1->location.lon.udeg == 11334500);
	QVERIFY(pic2->offset.seconds == 1321);

	learnPictureFilename(pic1->filename, PIC1_NAME);
	learnPictureFilename(pic2->filename, PIC2_NAME);
	QCOMPARE(localFilePath(pic1->filename), QString(PIC1_NAME));
	QCOMPARE(localFilePath(pic2->filename), QString(PIC2_NAME));
}


QTEST_GUILESS_MAIN(TestPicture)
