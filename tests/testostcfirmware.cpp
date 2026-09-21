// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#include "testostcfirmware.h"
#include "core/downloadfromdcthread.h"

void TestOstcFirmware::firmwareVersionComponentCounts()
{
	QString firmwareOnDevice;
	QVERIFY(OstcFirmwareCheck::isFirmwareVersionValid("OSTC 3", "1.2"));
	QVERIFY(OstcFirmwareCheck::isFirmwareVersionValid("OSTC Sport", "1.2"));
	QVERIFY(OstcFirmwareCheck::isFirmwareVersionValid("OSTC 4", "1.2.3"));
	QVERIFY(OstcFirmwareCheck::isFirmwareVersionValid("OSTC 5", "1.2.3"));
	QVERIFY(OstcFirmwareCheck::firmwareUpdateAvailable("OSTC 3", 0x0102, "1.3", &firmwareOnDevice));
	QVERIFY(OstcFirmwareCheck::firmwareUpdateAvailable("OSTC Sport", 0x0102, "1.3", &firmwareOnDevice));
	QVERIFY(OstcFirmwareCheck::firmwareUpdateAvailable("OSTC 4", 0x0886, "1.3.4", &firmwareOnDevice));
	QVERIFY(OstcFirmwareCheck::firmwareUpdateAvailable("OSTC 5", 0x0886, "1.3.4", &firmwareOnDevice));

	QVERIFY(!OstcFirmwareCheck::isFirmwareVersionValid("OSTC 3", "1"));
	QVERIFY(!OstcFirmwareCheck::isFirmwareVersionValid("OSTC Sport", "1.2.3"));
	QVERIFY(!OstcFirmwareCheck::isFirmwareVersionValid("OSTC 4", "1.2"));
	QVERIFY(!OstcFirmwareCheck::isFirmwareVersionValid("OSTC 5", "1.2.3.4"));
	QVERIFY(!OstcFirmwareCheck::isFirmwareVersionValid("OSTC 3", QString()));
	QVERIFY(!OstcFirmwareCheck::isFirmwareVersionValid("OSTC 4", "1..3"));
	QVERIFY(!OstcFirmwareCheck::isFirmwareVersionValid("OSTC 4", "1.two.3"));
	QVERIFY(!OstcFirmwareCheck::isFirmwareVersionValid("OSTC 4", "1.+2.3"));
	QVERIFY(!OstcFirmwareCheck::isFirmwareVersionValid("OSTC 3", "1.256"));
	QVERIFY(!OstcFirmwareCheck::isFirmwareVersionValid("OSTC 4", "32.2.3"));
	QVERIFY(!OstcFirmwareCheck::isFirmwareVersionValid("OSTC 4", "4294967296.2.3"));
}

void TestOstcFirmware::detectedOstc4RejectsOstc3Version()
{
	QString firmwareOnDevice;
	// AI-generated (Claude)
	// An OSTC 3 selection uses two-component firmware metadata. Once
	// DC_EVENT_DEVINFO has detected an OSTC 4, that metadata must not be used.
	QVERIFY(OstcFirmwareCheck::firmwareUpdateAvailable("OSTC 3", 0x0102, "1.3", &firmwareOnDevice));
	firmwareOnDevice.clear();
	QVERIFY(!OstcFirmwareCheck::firmwareUpdateAvailable("OSTC 4", 0x0886, "1.3", &firmwareOnDevice));
	QCOMPARE(firmwareOnDevice, QString());
}

QTEST_GUILESS_MAIN(TestOstcFirmware)
