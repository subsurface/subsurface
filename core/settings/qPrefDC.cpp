// SPDX-License-Identifier: GPL-2.0
#include "../pref.h"
#include "../qthelper.h"
#include "qPrefDC.h"
#include "qPref_private.h"


qPrefDC *qPrefDC::m_instance = NULL;
qPrefDC *qPrefDC::instance()
{
	if (!m_instance)
		m_instance = new qPrefDC;
	return m_instance;
}


void qPrefDC::loadSync(bool doSync)
{
	diskDevice(doSync);
	diskDeviceName(doSync);
	diskDownloadMode(doSync);
	diskProduct(doSync);
	diskVendor(doSync);
}

const QString qPrefDC::device() const
{
	return prefs.dive_computer.device;
}
void qPrefDC::setDevice(const QString& value)
{
	if (value != prefs.dive_computer.device) {
		COPY_TXT(dive_computer.device, value);
		diskDevice(true);
		emit deviceChanged(value);
	}
}
void qPrefDC::diskDevice(bool doSync)
{
	LOADSYNC_TXT("/dive_computer_device", dive_computer.device);
}


const QString qPrefDC::deviceName() const
{
	return prefs.dive_computer.device_name;
}
void qPrefDC::setDeviceName(const QString& value)
{
	if (value != prefs.dive_computer.device_name) {
		COPY_TXT(dive_computer.device_name, value);
		diskDeviceName(true);
		emit deviceNameChanged(value);
	}
}
void qPrefDC::diskDeviceName(bool doSync)
{
	LOADSYNC_TXT("/dive_computer_device_name", dive_computer.device_name);
}


int qPrefDC::downloadMode() const
{
	return prefs.dive_computer.download_mode;
}
void qPrefDC::setDownloadMode(int value)
{
	if (value != prefs.dive_computer.download_mode) {
		prefs.dive_computer.download_mode = value;
		diskDownloadMode(true);
		emit downloadModeChanged(value);
	}
}
void qPrefDC::diskDownloadMode(bool doSync)
{
	LOADSYNC_BOOL("/dive_computer_download_mode", dive_computer.download_mode);
}


const QString qPrefDC::product() const
{
	return prefs.dive_computer.product;
}
void qPrefDC::setProduct(const QString& value)
{
	if (value != prefs.dive_computer.product) {
		COPY_TXT(dive_computer.product, value);
		diskProduct(true);
		emit productChanged(value);
	}
}
void qPrefDC::diskProduct(bool doSync)
{
	LOADSYNC_TXT("/dive_computer_product", dive_computer.product);
}


const QString qPrefDC::vendor() const
{
	return prefs.dive_computer.vendor;
}
void qPrefDC::setVendor(const QString& value)
{
	if (value != prefs.dive_computer.vendor) {
		COPY_TXT(dive_computer.vendor, value);
		diskVendor(true);
		emit vendorChanged(value);
	}
}
void qPrefDC::diskVendor(bool doSync)
{
	LOADSYNC_TXT("/dive_computer_vendor", dive_computer.vendor);
}
