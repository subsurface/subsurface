// SPDX-License-Identifier: GPL-2.0

#include "btdiscovery.h"
#include "downloadfromdcthread.h"
#include "core/libdivecomputer.h"
#include <QTimer>
#include <QDebug>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QElapsedTimer>
#include <QCoreApplication>

extern QMap<QString, dc_descriptor_t *> descriptorLookup;

namespace {
	QHash<QString, QBluetoothDeviceInfo> btDeviceInfo;
}
BTDiscovery *BTDiscovery::m_instance = NULL;

struct modelPattern {
	uint16_t    model;
	const char *vendor;
	const char *product;
};
static struct modelPattern model[] = {
	{ 0x4552, "Oceanic", "Pro Plus X" },
	{ 0x455A, "Aqualung", "i750TC" },
	{ 0x4647, "Sherwood", "Sage" },
	{ 0x4648, "Aqualung", "i300C" },
	{ 0x4649, "Aqualung", "i200C" },
	{ 0x4651, "Aqualung", "i770R" },
	{ 0x4652, "Aqualung", "i550C" },
	{ 0x4653, "Oceanic", "Geo 4.0" },
	{ 0x4654, "Oceanic", "Veo 4.0" },
	{ 0x4655, "Sherwood", "Wisdom 4" },
	{ 0x4656, "Oceanic", "Pro Plus 4" },
	{ 0x4743, "Aqualung", "i470TC" }
};

struct namePattern {
	const char *prefix;
	const char *vendor;
	const char *product;
};
static struct namePattern name[] = {
	// Shearwater dive computers
	{ "Predator", "Shearwater", "Predator" },
	// both the Petrel and Petrel 2 identify as "Petrel" as BT/BLE device
	// but only the Petrel 2 is listed as available dive computer on iOS (which requires BLE support)
	// so always pick the "Petrel 2" as product when seeing a Petrel
	{ "Petrel", "Shearwater", "Petrel 2" },
	{ "Perdix", "Shearwater", "Perdix" },
	{ "Teric", "Shearwater", "Teric" },
	{ "Peregrine", "Shearwater", "Peregrine" },
	{ "NERD 2", "Shearwater", "NERD 2" },
	{ "NERD", "Shearwater", "NERD" }, // order is important, test for the more specific one first
	{ "Predator", "Shearwater", "Predator" },
	// Suunto dive computers
	{ "EON Steel", "Suunto", "EON Steel" },
	{ "EON Core", "Suunto", "EON Core" },
	{ "Suunto D5", "Suunto", "D5" },
	// Scubapro dive computers
	{ "G2", "Scubapro", "G2" },
	{ "HUD", "Scubapro", "G2 HUD" },
	{ "Aladin", "Scubapro", "Aladin Sport Matrix" },
	{ "A1", "Scubapro", "Aladin A1" },
	// Mares dive computers
	{ "Mares Genius", "Mares", "Genius" },
	{ "Mares", "Mares", "Quad" }, // we actually don't know and just pick a common one - user needs to fix in UI
	// Cress dive computers
	{ "CARESIO_", "Cressi", "Cartesio" },
	{ "GOA_", "Cressi", "Goa" },
	// Deepblu dive computesr
	{ "COSMIQ", "Deepblu", "Cosmiq+" },
	// Oceans dive computers
	{ "S1", "Oceans", "S1" },
	// McLean dive computers
	{ "McLean Extreme", "McLean", "Extreme" },
	// Tecdiving dive computers
	{ "DiveComputer", "Tecdiving", "DiveComputer.eu" }
};

static dc_descriptor_t *getDeviceType(QString btName)
// central function to convert a BT name to a Subsurface known vendor/model pair
{
	QString vendor, product;

	if (btName.startsWith("OSTC")) {
		vendor = "Heinrichs Weikamp";
		if (btName.mid(4,1) == "3") product = "OSTC Plus";
		else if (btName.mid(4,2) == "s#") product = "OSTC Sport";
		else if (btName.mid(4,2) == "s ") product = "OSTC Sport";
		else if (btName.mid(4,2) == "4-") product = "OSTC 4";
		else if (btName.mid(4,2) == "2-") product = "OSTC 2N";
		else if (btName.mid(4,2) == "+ ") product = "OSTC 2";
		// all BT/BLE enabled OSTCs are HW_FAMILY_OSTC_3, so when we do not know,
		// just use a default product that allows the codoe to download from the
		// user's dive computer
		else product = "OSTC 2";
	} else if (btName.contains(QRegularExpression("^DS\\d{6}"))) {
		// The Ratio bluetooth name looks like the Pelagic ones,
		// but that seems to be just happenstance.
		vendor = "Ratio";
		product = "iX3M GPS Easy"; // we don't know which of the GPS models, so set one
	} else if (btName.contains(QRegularExpression("^IX5M\\d{6}"))) {
		// The 2021 iX3M models (square buttons) report as iX5M,
		// eventhough the physical model states iX3M.
		vendor = "Ratio";
		product = "iX3M GPS Easy"; // we don't know which of the GPS models, so set one
	} else if (btName.contains(QRegularExpression("^[A-Z]{2}\\d{6}"))) {
		// try the Pelagic/Aqualung name patterns
		// the source of truth for this data is in libdivecomputer/src/descriptor.c
		// we'd prefer to use the filter functions there but current design makes that really challenging
		// The Pelagic dive computers (generally branded as Oceanic, Aqualung, or Sherwood)
		// show up with a two-byte model code followed by six bytes of serial
		// number. The model code matches the hex model (so "FQ" is 0x4651,
		// where 'F' is 46h and 'Q' is 51h in ASCII).
		for (uint16_t i = 0; i < sizeof(model) / sizeof(struct modelPattern); i++) {
			QString pattern = QString("^%1%2\\d{6}$").arg(QChar(model[i].model >> 8)).arg(QChar(model[i].model & 0xFF));
			if (btName.contains(QRegularExpression(pattern))) {
				vendor = model[i].vendor;
				product = model[i].product;
				break;
			}
		}
	} else { // finally try all the string prefix based ones
		for (uint16_t i = 0; i < sizeof(name) / sizeof(struct namePattern); i++) {
			if (btName.startsWith(name[i].prefix)) {
				vendor = name[i].vendor;
				product = name[i].product;
				break;
			}
		}
	}

	// check if we found a known dive computer
	if (!vendor.isEmpty() && !product.isEmpty()) {
		dc_descriptor_t *lookup = descriptorLookup.value(vendor.toLower() + product.toLower());
		if (!lookup)
			qWarning("known dive computer %s not found in descriptorLookup", qPrintable(QString(vendor + product)));
		return lookup;
	}

	return nullptr;
}

bool matchesKnownDiveComputerNames(QString btName)
{
	return getDeviceType(btName) != nullptr;
}

BTDiscovery::BTDiscovery(QObject*) : m_btValid(false),
	m_showNonDiveComputers(false),
	discoveryAgent(nullptr)
{
	if (m_instance) {
		qDebug() << "trying to create an additional BTDiscovery object";
		return;
	}
	m_instance = this;
#if defined(BT_SUPPORT)
	QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth* = true"));
	BTDiscoveryReDiscover();
#endif
}

void BTDiscovery::showNonDiveComputers(bool show)
{
	m_showNonDiveComputers = show;
}

void BTDiscovery::BTDiscoveryReDiscover()
{
#if !defined(Q_OS_IOS)
	qDebug() << "BTDiscoveryReDiscover: localBtDevice.isValid()" << localBtDevice.isValid();
	if (localBtDevice.isValid() &&
	    localBtDevice.hostMode() != QBluetoothLocalDevice::HostPoweredOff) {
		btPairedDevices.clear();
		qDebug() <<  "BTDiscoveryReDiscover: localDevice " + localBtDevice.name() + " is powered on, starting discovery";
#else
	// for iOS we can't use the localBtDevice as iOS is BLE only
	// we need to find some other way to test if Bluetooth is enabled, though
	// for now just hard-code it
	if (1) {
#endif
		m_btValid = true;

		if (discoveryAgent == nullptr) {
			discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
			discoveryAgent->setLowEnergyDiscoveryTimeout(3 * 60 * 1000); // search for three minutes
			connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &BTDiscovery::btDeviceDiscovered);
			connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &BTDiscovery::btDeviceDiscoveryFinished);
			connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled, this, &BTDiscovery::btDeviceDiscoveryFinished);
			connect(discoveryAgent, QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error),
				[this](QBluetoothDeviceDiscoveryAgent::Error error){
					qDebug() << "device discovery received error" << discoveryAgent->errorString();
				});
			qDebug() << "discovery methods" << (int)QBluetoothDeviceDiscoveryAgent::supportedDiscoveryMethods();
		}
#if defined(Q_OS_ANDROID)
		// on Android, we cannot scan for classic devices - we just get the paired ones
		qDebug() << "starting BLE discovery";
		discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
		getBluetoothDevices();
		// and add the paired devices to the internal data
		// So behaviour is same on Linux/Bluez stack and
		// Android/Java stack with respect to discovery
		for (int i = 0; i < btPairedDevices.length(); i++)
			btDeviceDiscoveredMain(btPairedDevices[i], true);
#else
		qDebug() << "starting BT/BLE discovery";
		discoveryAgent->start();
		for (int i = 0; i < btPairedDevices.length(); i++)
			qDebug() << "Paired =" << btPairedDevices[i].name << btPairedDevices[i].address;
#endif

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
	delete discoveryAgent;
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

void BTDiscovery::btDeviceDiscoveryFinished()
{
	qDebug() << "BT/BLE finished discovery";
	QList<QBluetoothDeviceInfo> devList = discoveryAgent->discoveredDevices();
	for (QBluetoothDeviceInfo device: devList) {
		qDebug() << device.name() << device.address().toString();
	}
}

void BTDiscovery::btDeviceDiscovered(const QBluetoothDeviceInfo &device)
{
	btPairedDevice this_d;
	this_d.address = markBLEAddress(&device);
	this_d.name = device.name();
	btPairedDevices.append(this_d);

	const auto serviceUuids = device.serviceUuids();
	for (QBluetoothUuid id: serviceUuids) {
		addBtUuid(id);
		qDebug() << id.toByteArray();
	}

#if defined(Q_OS_IOS) || defined(Q_OS_MACOS) || defined(Q_OS_WIN)
	// on Windows, macOS and iOS we need to scan in order to be able to access a device;
	// let's remember the information we scanned on this run so we can at least
	// refer back to it and don't need to open the separate scanning dialog every
	// time we try to download from a BT/BLE dive computer.
	saveBtDeviceInfo(btDeviceAddress(&device, false), device);
#endif

	btDeviceDiscoveredMain(this_d, false);
}

void BTDiscovery::btDeviceDiscoveredMain(const btPairedDevice &device, bool fromPaired)
{
	btVendorProduct btVP;

	QString newDevice;
	dc_descriptor_t *newDC = getDeviceType(device.name);
	if (newDC)
		newDevice = dc_descriptor_get_product(newDC);
	else
		newDevice = device.name;

	QString msg;
	msg = QString("%1 device: '%2' [%3]: ").arg(fromPaired ? "Paired" : "Discovered new").arg(newDevice).arg(device.address);
	if (newDC) {
		QString vendor = dc_descriptor_get_vendor(newDC);
		qDebug() << msg << "this could be a " + vendor;
		btVP.btpdi = device;
		btVP.dcDescriptor = newDC;
		btVP.vendorIdx = vendorList.indexOf(vendor);
		btVP.productIdx = productList[vendor].indexOf(newDevice);
		btDCs << btVP;
		connectionListModel.addAddress(newDevice + " " + device.address);
		return;
	}
	// Do we want only devices we recognize as dive computers?
	if (m_showNonDiveComputers) {
		if (!newDevice.isEmpty())
			newDevice += " ";
		connectionListModel.addAddress(newDevice + device.address);
	}
	qDebug() << msg << "not recognized as dive computer";
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
			// 1 means Classic. 2 means BLE, 3 means dual stack
			result.address = dev.callObjectMethod("getAddress","()Ljava/lang/String;").toString();
			result.name = dev.callObjectMethod("getName", "()Ljava/lang/String;").toString();
			if (btType & 1) { // DEVICE_TYPE_CLASSIC
				qDebug() << "paired BT classic device type" << btType << "with address" << result.address;
				btPairedDevices.append(result);
			}
			if (btType & 2) { // DEVICE_TYPE_LE
				result.address = QString("LE:%1").arg(result.address);
				qDebug() << "paired BLE device type" << btType << "with address" << result.address;
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

void BTDiscovery::discoverAddress(QString address)
{
	// if we have a discovery agent, check if we know about the address and if not
	// make sure we are looking for it
	// (if we don't have a discoveryAgent then likely BT is off or something else went wrong)
	if (!discoveryAgent)
		return;

	// let's make sure there is no device name mixed in with the address
	QString btAddress;
	btAddress = extractBluetoothAddress(address);

	if (!btDeviceInfo.keys().contains(address) && !discoveryAgent->isActive()) {
		qDebug() << "restarting discovery agent";
		discoveryAgent->start();
	}
}

void BTDiscovery::stopAgent()
{
	if (!discoveryAgent)
		return;
	qDebug() << "---> stopping the discovery agent";
	discoveryAgent->stop();
}

bool isBluetoothAddress(const QString &address)
{
	return extractBluetoothAddress(address) != QString();
}
QString extractBluetoothAddress(const QString &address)
{
	QRegularExpression re("(LE:)*([0-9A-F]{2}:[0-9A-F]{2}:[0-9A-F]{2}:[0-9A-F]{2}:[0-9A-F]{2}:[0-9A-F]{2}|{[0-9A-F]{8}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{12}})",
			      QRegularExpression::CaseInsensitiveOption);
	QRegularExpressionMatch m = re.match(address);
	return m.captured(0);
}

QString extractBluetoothNameAddress(const QString &address, QString &name)
{
	// sometimes our device text is of the form "name (address)", sometimes it's just "address"
	// let's simply return the address
	name = QString();
	QString extractedAddress = extractBluetoothAddress(address);
	if (extractedAddress == address.trimmed())
		return address;

	QRegularExpression re("^([^()]+)\\(([^)]*\\))$");
	QRegularExpressionMatch m = re.match(address);
	if (m.hasMatch()) {
		name = m.captured(1).trimmed();
		return extractedAddress;
	}
	qDebug() << "can't parse address" << address;
	return QString();
}

void saveBtDeviceInfo(const QString &devaddr, QBluetoothDeviceInfo deviceInfo)
{
	btDeviceInfo[devaddr] = deviceInfo;
}

QBluetoothDeviceInfo getBtDeviceInfo(const QString &devaddr)
{
	if (btDeviceInfo.contains(devaddr)) {
		BTDiscovery::instance()->stopAgent();
		return btDeviceInfo[devaddr];
	}
	if(!btDeviceInfo.keys().contains(devaddr)) {
		qDebug() << "still looking scan is still running, we should just wait for a few moments";
		// wait for a maximum of 30 more seconds
		// yes, that seems crazy, but on my Mac I see this take more than 20 seconds
		QElapsedTimer timer;
		timer.start();
		do {
			if (btDeviceInfo.keys().contains(devaddr)) {
				BTDiscovery::instance()->stopAgent();
				return btDeviceInfo[devaddr];
			}
			QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
			QThread::msleep(100);
		} while (timer.elapsed() < 30000);
	}
	qDebug() << "notify user that we can't find" << devaddr;
	return QBluetoothDeviceInfo();
}
#endif // BT_SUPPORT
