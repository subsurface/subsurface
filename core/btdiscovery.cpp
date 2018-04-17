// SPDX-License-Identifier: GPL-2.0

#include "btdiscovery.h"
#include "downloadfromdcthread.h"
#include "core/libdivecomputer.h"
#include <QTimer>
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
		else if (btName.mid(4,2) == "s ") product = "OSTC Sport";
		else if (btName.mid(4,2) == "4-") product = "OSTC 4";
		else if (btName.mid(4,2) == "2-") product = "OSTC 2N";
		// all OSTCs are HW_FAMILY_OSTC_3, so when we do not know,
		// just try this
		else product = "OSTC 3"; // all OSTCs are HW_FAMILY_OSTC_3
	}

	if (btName.startsWith("Petrel") || btName.startsWith("Perdix") || btName.startsWith("Predator")) {
		vendor = "Shearwater";
		if (btName.startsWith("Petrel")) product = "Petrel"; // or petrel 2?
		if (btName.startsWith("Perdix")) product = "Perdix";
		if (btName.startsWith("Predator")) product = "Predator";
	}

	if (btName.startsWith("EON Steel")) {
		vendor = "Suunto";
		product = "EON Steel";
	}

	if (btName.startsWith("G2")  || btName.startsWith("Aladin")) {
		vendor = "Scubapro";
		if (btName.startsWith("G2")) product = "G2";
		if (btName.startsWith("Aladin")) product = "Aladin Sport Matrix";
	}

	if (!vendor.isEmpty() && !product.isEmpty())
		return descriptorLookup.value(vendor + product);

	return NULL;
}

BTDiscovery::BTDiscovery(QObject *parent) : m_btValid(false),
	discoveryAgent(NULL)
{
	Q_UNUSED(parent)
	if (m_instance) {
		qDebug() << "trying to create an additional BTDiscovery object";
		return;
	}
	m_instance = this;
#if defined(BT_SUPPORT)
	BTDiscoveryReDiscover();
#endif
}

void BTDiscovery::BTDiscoveryReDiscover()
{
#if !defined(Q_OS_IOS)
	if (localBtDevice.isValid() &&
	    localBtDevice.hostMode() == QBluetoothLocalDevice::HostConnectable) {
		btPairedDevices.clear();
		qDebug() <<  "localDevice " + localBtDevice.name() + " is valid, starting discovery";
#else
	// for iOS we can't use the localBtDevice as iOS is BLE only
	// we need to find some other way to test if Bluetooth is enabled, though
	// for now just hard-code it
	if (1) {
#endif
		m_btValid = true;
#if defined(Q_OS_IOS) || (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID))
		discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
		connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &BTDiscovery::btDeviceDiscovered);
		qDebug() << "starting BLE discovery";
		discoveryAgent->start();
#endif
#if defined(Q_OS_ANDROID)
		getBluetoothDevices();
		// and add the paired devices to the internal data
		// So behaviour is same on Linux/Bluez stack and
		// Android/Java stack with respect to discovery
		for (int i = 0; i < btPairedDevices.length(); i++) {
			btDeviceDiscoveredMain(btPairedDevices[i]);

		}
#endif
		for (int i = 0; i < btPairedDevices.length(); i++) {
			qDebug() << "Paired =" << btPairedDevices[i].name << btPairedDevices[i].address;
		}
#if defined(Q_OS_IOS) || (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID))
		QTimer timer;
		timer.setSingleShot(true);
		connect(&timer, &QTimer::timeout, discoveryAgent, &QBluetoothDeviceDiscoveryAgent::stop);
		timer.start(3000);
#endif
	} else {
		qDebug() << "localBtDevice isn't valid or not connectable";
		m_btValid = false;
	}
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

QString btDeviceAddress(const QBluetoothDeviceInfo *device, bool isBle)
{
	QString address = device->address().isNull() ?
		device->deviceUuid().toString() : device->address().toString();
	const char *prefix = isBle ? "LE:" : "";
	return prefix + address;
}

QString markBLEAddress(const QBluetoothDeviceInfo *device)
{
	QBluetoothDeviceInfo::CoreConfigurations flags = device->coreConfigurations();
	bool isBle = flags == QBluetoothDeviceInfo::LowEnergyCoreConfiguration;

	return btDeviceAddress(device, isBle);
}

void BTDiscovery::btDeviceDiscovered(const QBluetoothDeviceInfo &device)
{
	btPairedDevice this_d;
	this_d.address = markBLEAddress(&device);
	this_d.name = device.name();
	btPairedDevices.append(this_d);

	QList<QBluetoothUuid> serviceUuids = device.serviceUuids();
	foreach (QBluetoothUuid id, serviceUuids) {
		addBtUuid(id);
		qDebug() << id.toByteArray();
	}

#if defined(Q_OS_IOS)
	// On Desktop this is called when "Save" button is clicked. All
	// DeviceInfo are stored as data on the ui list items.
	// On mobile (iOS) the current ConnectionListModel does not support
	// additional data, so just save all discovered devices.

	saveBtDeviceInfo(btDeviceAddress(&device, false), device);
#endif

	btDeviceDiscoveredMain(this_d);
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
	if (newDC) {
		QString vendor = dc_descriptor_get_vendor(newDC);
		qDebug() << "this could be a " + vendor + " " + newDevice;
		btVP.btpdi = device;
		btVP.dcDescriptor = newDC;
		btVP.vendorIdx = vendorList.indexOf(vendor);
		btVP.productIdx = productList[vendor].indexOf(newDevice);
		btDCs << btVP;
		connectionListModel.addAddress(newDevice + " " + device.address);
		return;
	}
	connectionListModel.addAddress(device.address);
	qDebug() << "Not recognized as dive computer";
}

QList<BTDiscovery::btVendorProduct> BTDiscovery::getBtDcs()
{
	return btDCs;
}

bool BTDiscovery::btAvailable() const
{
	return m_btValid;

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
			jint btType = dev.callMethod<jint>("getType", "()I");
			result.address = dev.callObjectMethod("getAddress","()Ljava/lang/String;").toString();
			if (btType == 2) // DEVICE_TYPE_LE
				result.address = QString("LE:%1").arg(result.address);
			result.name = dev.callObjectMethod("getName", "()Ljava/lang/String;").toString();
			qDebug() << "paired Device type" << btType << "with address" << result.address;
			btPairedDevices.append(result);
			if (btType == 3) { // DEVICE_TYPE_DUAL
				result.address = QString("LE:%1").arg(result.address);
				qDebug() << "paired Device type" << btType << "with address" << result.address;
				btPairedDevices.append(result);
			}
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

QHash<QString, QBluetoothDeviceInfo> btDeviceInfo;

void saveBtDeviceInfo(const QString &devaddr, QBluetoothDeviceInfo deviceInfo)
{
	btDeviceInfo[devaddr] = deviceInfo;
}

QBluetoothDeviceInfo getBtDeviceInfo(const QString &devaddr)
{
	if (btDeviceInfo.contains(devaddr))
		return btDeviceInfo[devaddr];
	qDebug() << "need to scan for" << devaddr;
	return QBluetoothDeviceInfo();
}
#endif // BT_SUPPORT
