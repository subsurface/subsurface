// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFSDIVECOMPUTER_H
#define QPREFSDIVECOMPUTER_H
#include "core/pref.h"

#include <QObject>

class qPrefDiveComputer : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString device READ device WRITE set_device NOTIFY deviceChanged);
	Q_PROPERTY(QString device_name READ device_name WRITE set_device_name NOTIFY device_nameChanged);
	Q_PROPERTY(int download_mode READ download_mode WRITE set_download_mode NOTIFY download_modeChanged);
	Q_PROPERTY(QString product READ product WRITE set_product NOTIFY productChanged);
	Q_PROPERTY(QString vendor READ vendor WRITE set_vendor NOTIFY vendorChanged);

public:
	qPrefDiveComputer(QObject *parent = NULL);
	static qPrefDiveComputer *instance();

	// Load/Sync local settings (disk) and struct preference
	static void loadSync(bool doSync);
	static void load() { loadSync(false); }
	static void sync() { loadSync(true); }

public:
	static QString device() { return prefs.dive_computer.device; }
	static QString device_name() { return prefs.dive_computer.device_name; }
	static int download_mode() { return prefs.dive_computer.download_mode; }
	static QString product() { return prefs.dive_computer.product; }
	static QString vendor() { return prefs.dive_computer.vendor; }

public slots:
	static void set_device(const QString &device);
	static void set_device_name(const QString &device_name);
	static void set_download_mode(int mode);
	static void set_product(const QString &product);
	static void set_vendor(const QString &vendor);

signals:
	void deviceChanged(const QString &device);
	void device_nameChanged(const QString &device_name);
	void download_modeChanged(int mode);
	void productChanged(const QString &product);
	void vendorChanged(const QString &vendor);

private:
	// functions to load/sync variable with disk
	static void disk_device(bool doSync);
	static void disk_device_name(bool doSync);
	static void disk_download_mode(bool doSync);
	static void disk_product(bool doSync);
	static void disk_vendor(bool doSync);
};

#endif
