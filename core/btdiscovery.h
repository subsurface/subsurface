// SPDX-License-Identifier: GPL-2.0
#ifndef BTDISCOVERY_H
#define BTDISCOVERY_H

#include <QObject>
#include <QString>
#include <QLoggingCategory>
#include <QAbstractListModel>
#include <QBluetoothLocalDevice>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothUuid>
#include "core/libdivecomputer.h"

#if defined(Q_OS_ANDROID)
#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>
#endif

void saveBtDeviceInfo(const QString &devaddr, QBluetoothDeviceInfo deviceInfo);
bool isBluetoothAddress(const QString &address);
bool matchesKnownDiveComputerNames(QString btName);
QString extractBluetoothAddress(const QString &address);
QString extractBluetoothNameAddress(const QString &address, QString &name);
QBluetoothDeviceInfo getBtDeviceInfo(const QString &devaddr);

class BTDiscovery : public QObject {
	Q_OBJECT

public:
	BTDiscovery(QObject *parent = NULL);
	~BTDiscovery();
	static BTDiscovery *instance();

	struct btPairedDevice {
		QString address;
		QString name;
	};

	struct btVendorProduct {
		btPairedDevice btpdi;
		dc_descriptor_t *dcDescriptor;
		int vendorIdx;
		int productIdx;
	};

	void btDeviceDiscoveryFinished();
	void btDeviceDiscovered(const QBluetoothDeviceInfo &device);
	void btDeviceDiscoveredMain(const btPairedDevice &device, bool fromPaired);
	bool btAvailable() const;
	void showNonDiveComputers(bool show);
	void stopAgent();

#if defined(Q_OS_ANDROID)
	void getBluetoothDevices();
#endif
	QList<btVendorProduct> getBtDcs();
	QBluetoothLocalDevice localBtDevice;
	void BTDiscoveryReDiscover();
	void discoverAddress(QString address);

private:
	static BTDiscovery *m_instance;
	bool m_btValid;
	bool m_showNonDiveComputers;

	QList<struct btVendorProduct> btDCs;		// recognized DCs
	QList<struct btVendorProduct> btAllDevices;	// all paired BT stuff

#if defined(Q_OS_ANDROID)
	bool checkException(const char* method, const QAndroidJniObject* obj);
#endif

	QList<struct btPairedDevice> btPairedDevices;
	QBluetoothDeviceDiscoveryAgent *discoveryAgent;

signals:
	void dcVendorChanged();
	void dcProductChanged();
	void dcBtChanged();
};
#endif // BTDISCOVERY_H
