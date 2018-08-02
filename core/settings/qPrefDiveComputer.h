// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFSDIVECOMPUTER_H
#define QPREFSDIVECOMPUTER_H
#include "core/pref.h"

#include <QObject>

class qPrefDiveComputer : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString device READ device WRITE set_device NOTIFY device_changed);
	Q_PROPERTY(QString device_name READ device_name WRITE set_device_name NOTIFY device_name_changed);
	Q_PROPERTY(int download_mode READ download_mode WRITE set_download_mode NOTIFY download_mode_changed);
	Q_PROPERTY(QString product READ product WRITE set_product NOTIFY product_changed);
	Q_PROPERTY(QString vendor READ vendor WRITE set_vendor NOTIFY vendor_changed);

public:
	qPrefDiveComputer(QObject *parent = NULL);
	static qPrefDiveComputer *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);
	void load() { loadSync(false); }
	void sync() { loadSync(true); }

public:
	static QString device() { return prefs.dive_computer.device; }
	static QString device_name() { return prefs.dive_computer.device_name; }
	static int download_mode() { return prefs.dive_computer.download_mode; }
	static QString product() { return prefs.dive_computer.product; }
	static QString vendor() { return prefs.dive_computer.vendor; }

public slots:
	void set_device(const QString &device);
	void set_device_name(const QString &device_name);
	void set_download_mode(int mode);
	void set_product(const QString &product);
	void set_vendor(const QString &vendor);

signals:
	void device_changed(const QString &device);
	void device_name_changed(const QString &device_name);
	void download_mode_changed(int mode);
	void product_changed(const QString &product);
	void vendor_changed(const QString &vendor);

private:
	// functions to load/sync variable with disk
	void disk_device(bool doSync);
	void disk_device_name(bool doSync);
	void disk_download_mode(bool doSync);
	void disk_product(bool doSync);
	void disk_vendor(bool doSync);
};

#endif
