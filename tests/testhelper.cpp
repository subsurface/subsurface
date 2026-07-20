// SPDX-License-Identifier: GPL-2.0
#include "testhelper.h"
#include "core/bluetoothaddress.h"
#include "core/btdiscovery.h"

void TestHelper::initTestCase()
{
	TestBase::initTestCase();
}

void TestHelper::recognizeBtAddress()
{
	QCOMPARE(isBluetoothAddress("01:a2:b3:c4:d5:06"), true);
	QCOMPARE(isBluetoothAddress("LE:01:A2:B3:C4:D5:06"), true);
	QCOMPARE(isBluetoothAddress("BT:01:A2:B3:C4:D5:06"), true);
	QCOMPARE(isBluetoothAddress("BT:{6e50ff5d-cdd3-4c43-a80a-1ed4c7d2d2a5}"), false);
	QCOMPARE(isBluetoothLowEnergyAddress("LE:01:A2:B3:C4:D5:06"), true);
	QCOMPARE(isBluetoothClassicAddress("BT:01:A2:B3:C4:D5:06"), true);
	QCOMPARE(isBluetoothClassicAddress("BT:{6e50ff5d-cdd3-4c43-a80a-1ed4c7d2d2a5}"), false);
	QCOMPARE(bluetoothAddressWithoutPrefix("BT:01:A2:B3:C4:D5:06"), QString("01:A2:B3:C4:D5:06"));
	QCOMPARE(bluetoothAddressWithoutPrefix("LE:01:A2:B3:C4:D5:06"), QString("01:A2:B3:C4:D5:06"));
	QCOMPARE(isBluetoothAddress("01A2B3C4D506"), true);
	QCOMPARE(isBluetoothAddress("01-A2-B3-C4-D5-06"), true);
	QCOMPARE(isBluetoothAddress("01:A2:b3:04:05"), false);
	QCOMPARE(isBluetoothAddress("LE:01:02:03:04:05"), false);
	QCOMPARE(isBluetoothAddress("01:02:03:04:051:67"), false);
	QCOMPARE(isBluetoothAddress("LE:01:g2:03:04:05"), false);
	QCOMPARE(isBluetoothAddress("LE:{6e50ff5d-cdd3-4c43-a80a-1ed4c7d2d2a5}"), true);
	QCOMPARE(isBluetoothAddress("LE:{6e5ff5d-cdd3-4c43-a80a-1ed4c7d2d2a5}"), false);
	QCOMPARE(isBluetoothAddress("LE:{6e50ff5d-cdda33-4c43-a80a-1ed4c7d2d2a5}"), false);
	QCOMPARE(isBluetoothAddress("LE:{6e50ff5d-cdd3-4c43-1ed4c7d2d2a5}"), false);
	QCOMPARE(isBluetoothAddress("LE:{6e50ff5d-cdd3-4c43-ag0a-1ed4c7d2d2a5}"), false);
}

void TestHelper::parseNameAddress()
{
	QString name, address;
	std::tie(address, name) = extractBluetoothNameAddress("01:a2:b3:c4:d5:06");
	QCOMPARE(address, QString("01:a2:b3:c4:d5:06"));
	QCOMPARE(name, QString());
	std::tie(address, name) = extractBluetoothNameAddress("  01:a2:b3:c4:d5:06  ");
	QCOMPARE(address, QString("01:a2:b3:c4:d5:06"));
	QCOMPARE(name, QString());
	std::tie(address, name) = extractBluetoothNameAddress("somename (01:a2:b3:c4:d5:06)");
	QCOMPARE(address, QString("01:a2:b3:c4:d5:06"));
	QCOMPARE(name, QString("somename"));
	std::tie(address, name) = extractBluetoothNameAddress("garbage");
	QCOMPARE(address, QString());
	QCOMPARE(name, QString());
	std::tie(address, name) = extractBluetoothNameAddress("somename (garbage)");
	QCOMPARE(address, QString());
	QCOMPARE(name, QString());
	std::tie(address, name) = extractBluetoothNameAddress("somename (LE:{6e50ff5d-cdd3-4c43-a80a-1ed4c7d2d2a5})");
	QCOMPARE(address, QString("LE:{6e50ff5d-cdd3-4c43-a80a-1ed4c7d2d2a5}"));
	QCOMPARE(name, QString("somename"));
	std::tie(address, name) = extractBluetoothNameAddress("somename (BT:01:a2:b3:c4:d5:06)");
	QCOMPARE(address, QString("BT:01:a2:b3:c4:d5:06"));
	QCOMPARE(name, QString("somename"));

}

void TestHelper::automaticBluetoothAddress()
{
	QBluetoothDeviceInfo macDevice(QBluetoothAddress("01:A2:B3:C4:D5:06"), "test", 0);
	macDevice.setCoreConfigurations(QBluetoothDeviceInfo::LowEnergyCoreConfiguration);
	QCOMPARE(btDeviceAddressForAuto(&macDevice), QString("01:A2:B3:C4:D5:06"));

	QBluetoothUuid uuid(QStringLiteral("{6e50ff5d-cdd3-4c43-a80a-1ed4c7d2d2a5}"));
	QBluetoothDeviceInfo uuidDevice(uuid, "test", 0);
	QCOMPARE(btDeviceAddressForAuto(&uuidDevice), QString("LE:") + uuid.toString());
}

QTEST_GUILESS_MAIN(TestHelper)
