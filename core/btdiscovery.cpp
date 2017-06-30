// SPDX-License-Identifier: GPL-2.0

#include "btdiscovery.h"
#include "downloadfromdcthread.h"
#include "core/libdivecomputer.h"
#include <QDebug>

extern QMap<QString, dc_descriptor_t *> descriptorLookup;

BTDiscovery *BTDiscovery::m_instance = NULL;

static dc_descriptor_t *getDeviceType(QString btName)
// central function to convert a BT name to a Subsurface known vendor/model pair
{
	QString vendor, product;

	if (btName.startsWith("OSTC")) {
		vendor = "Heinrichs Weikamp";
		if (btName.mid(4,2) == "3#") product = "OSTC 3";
		else if (btName.mid(4,2) == "3+") product = "OSTC 3+";
		else if (btName.mid(4,2) == "s#") product = "OSTC Sport";
		else if (btName.mid(4,2) == "4-") product = "OSTC 4";
		else if (btName.mid(4,2) == "2-") product = "OSTC 2N";
		// all OSTCs are HW_FAMILY_OSTC_3, so when we do not know,
		// just try this
		else product = "OSTC 3"; // all OSTCs are HW_FAMILY_OSTC_3
	}

	if (btName.startsWith("Petrel") || btName.startsWith("Perdix")) {
		vendor = "Shearwater";
		if (btName.startsWith("Petrel")) product = "Petrel"; // or petrel 2?
		if (btName.startsWith("Perdix")) product = "Perdix";
	}

	if (btName.startsWith("EON Steel")) {
		vendor = "Suunto";
		product = "EON Steel";
	}

	if (!vendor.isEmpty() && !product.isEmpty())
		return(descriptorLookup[vendor + product]);

	return(NULL);
}

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
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
		discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
		connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &BTDiscovery::btDeviceDiscovered);
		discoveryAgent->start();
#endif
#if defined(Q_OS_ANDROID) && defined(BT_SUPPORT)
		getBluetoothDevices();
		// and add the paired devices to the internal data
		// So behaviour is same on Linux/Bluez stack and
		// Android/Java stack with respect to discovery
		for (int i = 0; i < btPairedDevices.length(); i++) {
			btDeviceDiscoveredMain(btPairedDevices[i]);

		}
#endif
		for (int i = 0; i < btPairedDevices.length(); i++) {
			qDebug() << "Paired =" << btPairedDevices[i].name << btPairedDevices[i].address.toString();
		}
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
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
#if defined(SSRF_CUSTOM_IO)
extern void addBtUuid(QBluetoothUuid uuid);
#endif
extern QHash<QString, QStringList> productList;
extern QStringList vendorList;

QString markBLEAddress(const QBluetoothDeviceInfo *device)
{
	QBluetoothDeviceInfo::CoreConfigurations flags;
	QString prefix = "";

	flags = device->coreConfigurations();
	if (flags == QBluetoothDeviceInfo::LowEnergyCoreConfiguration)
		prefix = "LE:";

	return prefix + device->address().toString();
}

void BTDiscovery::btDeviceDiscovered(const QBluetoothDeviceInfo &device)
{
#if defined(SSRF_CUSTOM_IO)
	btPairedDevice this_d;
	this_d.address = device.address();
	this_d.name = device.name();
	btPairedDevices.append(this_d);

	QList<QBluetoothUuid> serviceUuids = device.serviceUuids();
	foreach (QBluetoothUuid id, serviceUuids) {
		addBtUuid(id);
		qDebug() << id.toByteArray();
	}

	btDeviceDiscoveredMain(this_d);
#endif
}

void BTDiscovery::btDeviceDiscoveredMain(const btPairedDevice &device)
{
	btVendorProduct btVP;

	QString newDevice;
	dc_descriptor_t *newDC = getDeviceType(device.name);
	if (newDC)
		newDevice = dc_descriptor_get_product(newDC);
	 else
		newDevice = device.name;

	qDebug() << "Found new device:" << newDevice << device.address;
	QString vendor;
	if (newDC) foreach (vendor, productList.keys()) {
		if (productList[vendor].contains(newDevice)) {
			qDebug() << "this could be a " + vendor + " " +
					(newDevice == "OSTC 3" ? "OSTC family" : newDevice);
			btVP.btpdi = device;
			btVP.dcDescriptor = newDC;
			btVP.vendorIdx = vendorList.indexOf(vendor);
			btVP.productIdx = productList[vendor].indexOf(newDevice);
			qDebug() << "adding new btDCs entry (detected DC)" << newDevice << btVP.vendorIdx << btVP.productIdx << btVP.btpdi.address;;
			btDCs << btVP;
			break;
		}
	}
	productList[QObject::tr("Paired Bluetooth Devices")].append(device.name + " (" + device.address.toString() + ")");

	btVP.btpdi = device;
	btVP.dcDescriptor = newDC;
	btVP.vendorIdx = vendorList.indexOf(QObject::tr("Paired Bluetooth Devices"));
	btVP.productIdx = productList[QObject::tr("Paired Bluetooth Devices")].indexOf(device.name);
	qDebug() << "adding new btDCs entry (all paired)" << newDevice << btVP.vendorIdx << btVP.productIdx <<  btVP.btpdi.address;
	btAllDevices << btVP;
}

QList<BTDiscovery::btVendorProduct> BTDiscovery::getBtDcs()
{
	return btDCs;
}

QList <BTDiscovery::btVendorProduct> BTDiscovery::getBtAllDevices()
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
