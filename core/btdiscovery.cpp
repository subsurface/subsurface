// SPDX-License-Identifier: GPL-2.0

#include "btdiscovery.h"
#include "downloadfromdcthread.h"
#include <QDebug>

BTDiscovery *BTDiscovery::m_instance = NULL;

BTDiscovery::BTDiscovery(QObject *parent)
{
	Q_UNUSED(parent)
	if (m_instance) {
		qDebug() << "trying to create an additional BTDiscovery object";
		return;
	}
	m_instance = this;
#if defined(BT_SUPPORT)
	if (localBtDevice.isValid() &&
	    localBtDevice.hostMode() == QBluetoothLocalDevice::HostConnectable) {
		btPairedDevices.clear();
		qDebug() <<  "localDevice " + localBtDevice.name() + " is valid, starting discovery";
#if defined(Q_OS_LINUX)
		discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
		connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &BTDiscovery::btDeviceDiscovered);
		discoveryAgent->start();
#endif
#if defined(Q_OS_ANDROID) && defined(BT_SUPPORT)
		getBluetoothDevices();
#endif
		for (int i = 0; i < btPairedDevices.length(); i++) {
			qDebug() << "Paired =" << btPairedDevices[i].name << btPairedDevices[i].address.toString();
		}
#if defined(Q_OS_LINUX)
		discoveryAgent->stop();
#endif
	} else {
		qDebug() << "localBtDevice isn't valid";
	}
#endif
}

BTDiscovery::~BTDiscovery()
{
	m_instance = NULL;
#if defined(BT_SUPPORT)
	free(discoveryAgent);
#endif
}

BTDiscovery *BTDiscovery::instance()
{
	if (!m_instance)
		m_instance = new BTDiscovery();
	return m_instance;
}

#if defined(BT_SUPPORT)

extern void addBtUuid(QBluetoothUuid uuid);
extern QHash<QString, QStringList> productList;
extern QStringList vendorList;

void BTDiscovery::btDeviceDiscovered(const QBluetoothDeviceInfo &device)
{
	btPairedDevice this_d;
	this_d.address = device.address();
	this_d.name = device.name();
	btPairedDevices.append(this_d);
	struct btVendorProduct btVP;

	QString newDevice = device.name();

	// all the HW OSTC BT computers show up as "OSTC" + some other text, depending on model
	if (newDevice.startsWith("OSTC"))
		newDevice = "OSTC 3";
	QList<QBluetoothUuid> serviceUuids = device.serviceUuids();
	foreach (QBluetoothUuid id, serviceUuids) {
		addBtUuid(id);
		qDebug() << id.toByteArray();
	}
	qDebug() << "Found new device:" << newDevice << device.address();
	QString vendor;
	foreach (vendor, productList.keys()) {
		if (productList[vendor].contains(newDevice)) {
			qDebug() << "this could be a " + vendor + " " +
					(newDevice == "OSTC 3" ? "OSTC family" : newDevice);
			btVP.btdi = device;
			btVP.vendorIdx = vendorList.indexOf(vendor);
			btVP.productIdx = productList[vendor].indexOf(newDevice);
			qDebug() << "adding new btDCs entry (detected DC)" << newDevice << btVP.vendorIdx << btVP.productIdx << btVP.btdi.address();;
			btDCs << btVP;
			break;
		}
	}
	productList[QObject::tr("Paired Bluetooth Devices")].append(this_d.name + " (" + this_d.address.toString() + ")");

	btVP.btdi = device;
	btVP.vendorIdx = vendorList.indexOf(QObject::tr("Paired Bluetooth Devices"));
	btVP.productIdx = productList[QObject::tr("Paired Bluetooth Devices")].indexOf(this_d.name);
	qDebug() << "adding new btDCs entry (all paired)" << newDevice << btVP.vendorIdx << btVP.productIdx <<  btVP.btdi.address();
	btAllDevices << btVP;
}

QList <struct btVendorProduct> BTDiscovery::getBtDcs()
{
	return btDCs;
}

QList <struct btVendorProduct> BTDiscovery::getBtAllDevices()
{
	return btAllDevices;
}

// Android: As Qt is not able to pull the pairing data from a device, i
// a lengthy discovery process is needed to see what devices are paired. On
// https://forum.qt.io/topic/46075/solved-bluetooth-list-paired-devices
// user s.frings74 does, however, present a solution to this using JNI.
// Currently, this code is taken "as is".

#if defined(Q_OS_ANDROID)
void BTDiscovery::getBluetoothDevices()
{
	struct BTDiscovery::btPairedDevice result;
	// Query via Android Java API.

	// returns a BluetoothAdapter
	QAndroidJniObject adapter=QAndroidJniObject::callStaticObjectMethod("android/bluetooth/BluetoothAdapter","getDefaultAdapter","()Landroid/bluetooth/BluetoothAdapter;");
	if (checkException("BluetoothAdapter.getDefaultAdapter()", &adapter)) {
		return;
	}
	// returns a Set<BluetoothDevice>
	QAndroidJniObject pairedDevicesSet=adapter.callObjectMethod("getBondedDevices","()Ljava/util/Set;");
	if (checkException("BluetoothAdapter.getBondedDevices()", &pairedDevicesSet)) {
		return;
	}
	jint size=pairedDevicesSet.callMethod<jint>("size");
	checkException("Set<BluetoothDevice>.size()", &pairedDevicesSet);
	if (size > 0) {
		// returns an Iterator<BluetoothDevice>
		QAndroidJniObject iterator=pairedDevicesSet.callObjectMethod("iterator","()Ljava/util/Iterator;");
		if (checkException("Set<BluetoothDevice>.iterator()", &iterator)) {
			return;
		}
		for (int i = 0; i < size; i++) {
			// returns a BluetoothDevice
			QAndroidJniObject dev=iterator.callObjectMethod("next","()Ljava/lang/Object;");
			if (checkException("Iterator<BluetoothDevice>.next()", &dev)) {
				continue;
			}

			result.address = QBluetoothAddress(dev.callObjectMethod("getAddress","()Ljava/lang/String;").toString());
			result.name = dev.callObjectMethod("getName", "()Ljava/lang/String;").toString();

			btPairedDevices.append(result);
		}
	}
}

bool BTDiscovery::checkException(const char* method, const QAndroidJniObject *obj)
{
	static QAndroidJniEnvironment env;
	bool result = false;

	if (env->ExceptionCheck()) {
		qCritical("Exception in %s", method);
		env->ExceptionDescribe();
		env->ExceptionClear();
		result=true;
	}
	if (!(obj == NULL || obj->isValid())) {
		qCritical("Invalid object returned by %s", method);
	result=true;
	}
	return result;
}
#endif // Q_OS_ANDROID
#endif // BT_SUPPORT
