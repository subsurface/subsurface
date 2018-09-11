// SPDX-License-Identifier: GPL-2.0
#include "qPrefDiveComputer.h"
#include "qPrefPrivate.h"

static const QString group = QStringLiteral("DiveComputer");

qPrefDiveComputer::qPrefDiveComputer(QObject *parent) : QObject(parent)
{
}

qPrefDiveComputer *qPrefDiveComputer::instance()
{
	static qPrefDiveComputer *self = new qPrefDiveComputer;
	return self;
}


void qPrefDiveComputer::loadSync(bool doSync)
{
	disk_device(doSync);
	disk_device_name(doSync);
	disk_download_mode(doSync);
	disk_product(doSync);
	disk_vendor(doSync);
}

HANDLE_PREFERENCE_TXT_EXT(DiveComputer, "dive_computer_device", device, dive_computer.);

HANDLE_PREFERENCE_TXT_EXT(DiveComputer, "dive_computer_device_name", device_name, dive_computer.);

HANDLE_PREFERENCE_INT_EXT(DiveComputer, "dive_computer_download_mode", download_mode, dive_computer.);

HANDLE_PREFERENCE_TXT_EXT(DiveComputer, "dive_computer_product", product, dive_computer.);

HANDLE_PREFERENCE_TXT_EXT(DiveComputer, "dive_computer_vendor", vendor, dive_computer.);
