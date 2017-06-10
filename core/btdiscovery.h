// SPDX-License-Identifier: GPL-2.0
#ifndef BTDISCOVERY_H
#define BTDISCOVERY_H

#include <QObject>
#include <QString>
#include <QLoggingCategory>
#if defined(BT_SUPPORT)
#include <QBluetoothLocalDevice>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothUuid>

struct btVendorProduct {
	QBluetoothDeviceInfo btdi;
	int vendorIdx;
	int productIdx;
};

#endif
#if defined(Q_OS_ANDROID)
#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>
#endif

class BTDiscovery : public QObject {
	Q_OBJECT

public:
	BTDiscovery(QObject *parent = NULL);
	~BTDiscovery();
	static BTDiscovery *instance();

#if defined(BT_SUPPORT)
	struct btPairedDevice {
		QBluetoothAddress address;
		QString name;
	};
	void btDeviceDiscovered(const QBluetoothDeviceInfo &device);
#if defined(Q_OS_ANDROID)
	void getBluetoothDevices();
#endif
	QList<struct btVendorProduct> getBtDcs();
	QList<struct btVendorProduct> getBtAllDevices();
#endif
private:
	static BTDiscovery *m_instance;
#if defined(BT_SUPPORT)
	QList<struct btVendorProduct> btDCs;		// recognized DCs
	QList<struct btVendorProduct> btAllDevices;	// all paired BT stuff
#endif
#if defined(Q_OS_ANDROID)
	bool checkException(const char* method, const QAndroidJniObject* obj);
#endif

#if defined(BT_SUPPORT)
	QList<struct btPairedDevice> btPairedDevices;
	QBluetoothLocalDevice localBtDevice;
	QBluetoothDeviceDiscoveryAgent *discoveryAgent;
#endif

signals:
	void dcVendorChanged();
	void dcProductChanged();
	void dcBtChanged();
};

#endif // BTDISCOVERY_H
