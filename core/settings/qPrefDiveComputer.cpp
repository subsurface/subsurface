// SPDX-License-Identifier: GPL-2.0
#include "qPrefDiveComputer.h"
#include "qPref_private.h"


qPrefDiveComputer *qPrefDiveComputer::m_instance = NULL;
qPrefDiveComputer *qPrefDiveComputer::instance()
{
	if (!m_instance)
		m_instance = new qPrefDiveComputer;
	return m_instance;
}


void qPrefDiveComputer::loadSync(bool doSync)
{
	diskDevice(doSync);
	diskDeviceName(doSync);
	diskDownloadMode(doSync);
	diskProduct(doSync);
	diskVendor(doSync);
}

const QString qPrefDiveComputer::device() const
{
	return prefs.dive_computer.device;
}
void qPrefDiveComputer::setDevice(const QString& value)
{
	if (value != prefs.dive_computer.device) {
		COPY_TXT(dive_computer.device, value);
		diskDevice(true);
		emit deviceChanged(value);
	}
}
void qPrefDiveComputer::diskDevice(bool doSync)
{
	LOADSYNC_TXT("/dive_computer_device", dive_computer.device);
}


const QString qPrefDiveComputer::device_name() const
{
	return prefs.dive_computer.device_name;
}
void qPrefDiveComputer::setDeviceName(const QString& value)
{
	if (value != prefs.dive_computer.device_name) {
		COPY_TXT(dive_computer.device_name, value);
		diskDeviceName(true);
		emit deviceNameChanged(value);
	}
}
void qPrefDiveComputer::diskDeviceName(bool doSync)
{
	LOADSYNC_TXT("/dive_computer_device_name", dive_computer.device_name);
}


int qPrefDiveComputer::downloadMode() const
{
	return prefs.dive_computer.download_mode;
}
void qPrefDiveComputer::setDownloadMode(int value)
{
	if (value != prefs.dive_computer.download_mode) {
		prefs.dive_computer.download_mode = value;
		diskDownloadMode(true);
		emit downloadModeChanged(value);
	}
}
void qPrefDiveComputer::diskDownloadMode(bool doSync)
{
	LOADSYNC_BOOL("/dive_computer_download_mode", dive_computer.download_mode);
}


const QString qPrefDiveComputer::product() const
{
	return prefs.dive_computer.product;
}
void qPrefDiveComputer::setProduct(const QString& value)
{
	if (value != prefs.dive_computer.product) {
		COPY_TXT(dive_computer.product, value);
		diskProduct(true);
		emit productChanged(value);
	}
}
void qPrefDiveComputer::diskProduct(bool doSync)
{
	LOADSYNC_TXT("/dive_computer_product", dive_computer.product);
}


const QString qPrefDiveComputer::vendor() const
{
	return prefs.dive_computer.vendor;
}
void qPrefDiveComputer::setVendor(const QString& value)
{
	if (value != prefs.dive_computer.vendor) {
		COPY_TXT(dive_computer.vendor, value);
		diskVendor(true);
		emit vendorChanged(value);
	}
}
void qPrefDiveComputer::diskVendor(bool doSync)
{
	LOADSYNC_TXT("/dive_computer_vendor", dive_computer.vendor);
}
