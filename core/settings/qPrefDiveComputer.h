// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFSDIVECOMPUTER_H
#define QPREFSDIVECOMPUTER_H
#include "core/pref.h"

#include <QObject>

#define IMPLEMENT5GETTERS(name) \
	static QString name() { return QString::fromStdString(prefs.dive_computer.name); } \
	static QString name##1() { return QString::fromStdString(prefs.dive_computer##1 .name); } \
	static QString name##2() { return QString::fromStdString(prefs.dive_computer##2 .name); } \
	static QString name##3() { return QString::fromStdString(prefs.dive_computer##3 .name); } \
	static QString name##4() { return QString::fromStdString(prefs.dive_computer##4 .name); }

class qPrefDiveComputer : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString device READ device WRITE set_device NOTIFY deviceChanged)
	Q_PROPERTY(QString device1 READ device1 WRITE set_device1 NOTIFY device1Changed)
	Q_PROPERTY(QString device2 READ device2 WRITE set_device2 NOTIFY device2Changed)
	Q_PROPERTY(QString device3 READ device3 WRITE set_device3 NOTIFY device3Changed)
	Q_PROPERTY(QString device4 READ device4 WRITE set_device4 NOTIFY device4Changed)
	Q_PROPERTY(QString device_name READ device_name WRITE set_device_name NOTIFY device_nameChanged)
	Q_PROPERTY(QString device_name1 READ device_name1 WRITE set_device_name1 NOTIFY device_name1Changed)
	Q_PROPERTY(QString device_name2 READ device_name2 WRITE set_device_name2 NOTIFY device_name2Changed)
	Q_PROPERTY(QString device_name3 READ device_name3 WRITE set_device_name3 NOTIFY device_name3Changed)
	Q_PROPERTY(QString device_name4 READ device_name4 WRITE set_device_name4 NOTIFY device_name4Changed)
	Q_PROPERTY(QString product READ product WRITE set_product NOTIFY productChanged)
	Q_PROPERTY(QString product1 READ product1 WRITE set_product1 NOTIFY product1Changed)
	Q_PROPERTY(QString product2 READ product2 WRITE set_product2 NOTIFY product2Changed)
	Q_PROPERTY(QString product3 READ product3 WRITE set_product3 NOTIFY product3Changed)
	Q_PROPERTY(QString product4 READ product4 WRITE set_product4 NOTIFY product4Changed)
	Q_PROPERTY(QString vendor READ vendor WRITE set_vendor NOTIFY vendorChanged)
	Q_PROPERTY(QString vendor1 READ vendor1 WRITE set_vendor1 NOTIFY vendor1Changed)
	Q_PROPERTY(QString vendor2 READ vendor2 WRITE set_vendor2 NOTIFY vendor2Changed)
	Q_PROPERTY(QString vendor3 READ vendor3 WRITE set_vendor3 NOTIFY vendor3Changed)
	Q_PROPERTY(QString vendor4 READ vendor4 WRITE set_vendor4 NOTIFY vendor4Changed)

	Q_PROPERTY(bool sync_dc_time READ sync_dc_time WRITE set_sync_dc_time NOTIFY sync_dc_timeChanged)

public:
	static qPrefDiveComputer *instance();

	// Load/Sync local settings (disk) and struct preference
	static void loadSync(bool doSync);
	static void load() { loadSync(false); }
	static void sync() { loadSync(true); }

public:
	IMPLEMENT5GETTERS(device)
	IMPLEMENT5GETTERS(device_name)
	IMPLEMENT5GETTERS(product)
	IMPLEMENT5GETTERS(vendor)

	static bool sync_dc_time() { return prefs.sync_dc_time; }

public slots:
	static void set_device(const QString &device);
	static void set_device1(const QString &device);
	static void set_device2(const QString &device);
	static void set_device3(const QString &device);
	static void set_device4(const QString &device);

	static void set_device_name(const QString &device_name);
	static void set_device_name1(const QString &device_name);
	static void set_device_name2(const QString &device_name);
	static void set_device_name3(const QString &device_name);
	static void set_device_name4(const QString &device_name);

	static void set_product(const QString &product);
	static void set_product1(const QString &product);
	static void set_product2(const QString &product);
	static void set_product3(const QString &product);
	static void set_product4(const QString &product);

	static void set_vendor(const QString &vendor);
	static void set_vendor1(const QString &vendor);
	static void set_vendor2(const QString &vendor);
	static void set_vendor3(const QString &vendor);
	static void set_vendor4(const QString &vendor);

	static void set_sync_dc_time(bool value);

signals:
	void deviceChanged(const QString &device);
	void device1Changed(const QString &device);
	void device2Changed(const QString &device);
	void device3Changed(const QString &device);
	void device4Changed(const QString &device);

	void device_nameChanged(const QString &device_name);
	void device_name1Changed(const QString &device_name);
	void device_name2Changed(const QString &device_name);
	void device_name3Changed(const QString &device_name);
	void device_name4Changed(const QString &device_name);

	void productChanged(const QString &product);
	void product1Changed(const QString &product);
	void product2Changed(const QString &product);
	void product3Changed(const QString &product);
	void product4Changed(const QString &product);

	void vendorChanged(const QString &vendor);
	void vendor1Changed(const QString &vendor);
	void vendor2Changed(const QString &vendor);
	void vendor3Changed(const QString &vendor);
	void vendor4Changed(const QString &vendor);

	void sync_dc_timeChanged(bool value);

private:
	qPrefDiveComputer() {}

	// functions to load/sync variable with disk

	static void disk_device(bool doSync);
	static void disk_device1(bool doSync);
	static void disk_device2(bool doSync);
	static void disk_device3(bool doSync);
	static void disk_device4(bool doSync);

	static void disk_device_name(bool doSync);
	static void disk_device_name1(bool doSync);
	static void disk_device_name2(bool doSync);
	static void disk_device_name3(bool doSync);
	static void disk_device_name4(bool doSync);

	static void disk_product(bool doSync);
	static void disk_product1(bool doSync);
	static void disk_product2(bool doSync);
	static void disk_product3(bool doSync);
	static void disk_product4(bool doSync);

	static void disk_vendor(bool doSync);
	static void disk_vendor1(bool doSync);
	static void disk_vendor2(bool doSync);
	static void disk_vendor3(bool doSync);
	static void disk_vendor4(bool doSync);

	static void disk_sync_dc_time(bool doSync);
};

#endif
