// SPDX-License-Identifier: GPL-2.0
#include "qPrefDiveComputer.h"
#include "qPrefPrivate.h"

static const QString group = QStringLiteral("DiveComputer");

qPrefDiveComputer *qPrefDiveComputer::instance()
{
	static qPrefDiveComputer *self = new qPrefDiveComputer;
	return self;
}

void qPrefDiveComputer::loadSync(bool doSync)
{
	// last computer used
	disk_device(doSync);
	disk_device_name(doSync);
	disk_product(doSync);
	disk_vendor(doSync);

	// the four shortcuts
#define DISK_DC(num) \
	disk_device##num(doSync); \
	disk_device_name##num(doSync); \
	disk_product##num(doSync); \
	disk_vendor##num(doSync);

	DISK_DC(1)
	DISK_DC(2)
	DISK_DC(3)
	DISK_DC(4)

	disk_sync_dc_time(doSync);
}

// these are the 'active' settings
HANDLE_PREFERENCE_TXT_EXT(DiveComputer, "dive_computer_device", device, dive_computer.)
HANDLE_PREFERENCE_TXT_EXT(DiveComputer, "dive_computer_device_name", device_name, dive_computer.)
HANDLE_PREFERENCE_TXT_EXT(DiveComputer, "dive_computer_product", product, dive_computer.)
HANDLE_PREFERENCE_TXT_EXT(DiveComputer, "dive_computer_vendor", vendor, dive_computer.)

// these are the previous four to make it easy to go back and forth between multiple dive computers
HANDLE_PREFERENCE_TXT_EXT_ALT(DiveComputer, "dive_computer_device1", device, dive_computer, 1)
HANDLE_PREFERENCE_TXT_EXT_ALT(DiveComputer, "dive_computer_device2", device, dive_computer, 2)
HANDLE_PREFERENCE_TXT_EXT_ALT(DiveComputer, "dive_computer_device3", device, dive_computer, 3)
HANDLE_PREFERENCE_TXT_EXT_ALT(DiveComputer, "dive_computer_device4", device, dive_computer, 4)
HANDLE_PREFERENCE_TXT_EXT_ALT(DiveComputer, "dive_computer_device_name1", device_name, dive_computer, 1)
HANDLE_PREFERENCE_TXT_EXT_ALT(DiveComputer, "dive_computer_device_name2", device_name, dive_computer, 2)
HANDLE_PREFERENCE_TXT_EXT_ALT(DiveComputer, "dive_computer_device_name3", device_name, dive_computer, 3)
HANDLE_PREFERENCE_TXT_EXT_ALT(DiveComputer, "dive_computer_device_name4", device_name, dive_computer, 4)
HANDLE_PREFERENCE_TXT_EXT_ALT(DiveComputer, "dive_computer_product1", product, dive_computer, 1)
HANDLE_PREFERENCE_TXT_EXT_ALT(DiveComputer, "dive_computer_product2", product, dive_computer, 2)
HANDLE_PREFERENCE_TXT_EXT_ALT(DiveComputer, "dive_computer_product3", product, dive_computer, 3)
HANDLE_PREFERENCE_TXT_EXT_ALT(DiveComputer, "dive_computer_product4", product, dive_computer, 4)
HANDLE_PREFERENCE_TXT_EXT_ALT(DiveComputer, "dive_computer_vendor1", vendor, dive_computer, 1)
HANDLE_PREFERENCE_TXT_EXT_ALT(DiveComputer, "dive_computer_vendor2", vendor, dive_computer, 2)
HANDLE_PREFERENCE_TXT_EXT_ALT(DiveComputer, "dive_computer_vendor3", vendor, dive_computer, 3)
HANDLE_PREFERENCE_TXT_EXT_ALT(DiveComputer, "dive_computer_vendor4", vendor, dive_computer, 4)

HANDLE_PREFERENCE_BOOL(DiveComputer, "sync_dive_computer_time", sync_dc_time);
