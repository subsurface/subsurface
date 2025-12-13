// SPDX-License-Identifier: GPL-2.0

#include "btdiscovery.h"
#include "downloadfromdcthread.h"
#include "libdivecomputer.h"
#include "errorhelper.h"
#include <QTimer>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QElapsedTimer>
#include <QCoreApplication>

extern QMap<QString, dc_descriptor_t *> descriptorLookup;

namespace {
	QHash<QString, QBluetoothDeviceInfo> btDeviceInfo;
}
BTDiscovery *BTDiscovery::m_instance = NULL;

static dc_descriptor_t *getDeviceType(QString btName, int transports)
// central function to convert a BT name to a Subsurface known vendor/model pair
{
	dc_status_t status = DC_STATUS_SUCCESS;
	dc_descriptor_t *result = NULL;
	const QByteArray tmp = btName.toLocal8Bit();
	const char *namestr = tmp.constData();

	dc_iterator_t *iterator = NULL;
	status = dc_descriptor_iterator_new (&iterator, NULL);
	if (status != DC_STATUS_SUCCESS) {
		report_error ("Error creating the device descriptor iterator.");
		return NULL;
	}

	dc_descriptor_t *descriptor = NULL;
	while ((status = dc_iterator_next (iterator, &descriptor)) == DC_STATUS_SUCCESS) {
		if (transports & DC_TRANSPORT_BLE) {
			if (dc_descriptor_filter(descriptor, DC_TRANSPORT_BLE, namestr)) {
				result = descriptor;
				break;
			}
		}
		if (transports & DC_TRANSPORT_BLUETOOTH) {
			if (dc_descriptor_filter(descriptor, DC_TRANSPORT_BLUETOOTH, namestr)) {
				result = descriptor;
				break;
			}
		}

		dc_descriptor_free (descriptor);
	}

	dc_iterator_free (iterator);

	if (status != DC_STATUS_SUCCESS && status != DC_STATUS_DONE) {
		report_error ("Error iterating the device descriptors.");
		return NULL;
	}

	return result;
}

bool matchesKnownDiveComputerNames(QString btName)
{
	return getDeviceType(btName, DC_TRANSPORT_BLUETOOTH | DC_TRANSPORT_BLE) != nullptr;
}

BTDiscovery::BTDiscovery(QObject*) : m_btValid(false),
	m_showNonDiveComputers(false),
	discoveryAgent(nullptr)
{
	if (m_instance) {
		report_info("trying to create an additional BTDiscovery object");
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
	report_info("BTDiscoveryReDiscover: localBtDevice.isValid() %d", localBtDevice.isValid());
	if (localBtDevice.isValid() &&
	    localBtDevice.hostMode() != QBluetoothLocalDevice::HostPoweredOff) {
		btPairedDevices.clear();
		report_info("BTDiscoveryReDiscover: localDevice %s is powered on, starting discovery", qPrintable(localBtDevice.name()));
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
			connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
#else
			connect(discoveryAgent, QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error),
#endif
				[this](QBluetoothDeviceDiscoveryAgent::Error error){
					report_info("device discovery received error %s", qPrintable(discoveryAgent->errorString()));
				});
			report_info("discovery methods %d", (int)QBluetoothDeviceDiscoveryAgent::supportedDiscoveryMethods());
		}
#if defined(Q_OS_ANDROID)
		// on Android, we cannot scan for classic devices - we just get the paired ones
		report_info("starting BLE discovery");
		discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
		getBluetoothDevices();
		// and add the paired devices to the internal data
		// So behaviour is same on Linux/Bluez stack and
		// Android/Java stack with respect to discovery
		for (int i = 0; i < btPairedDevices.length(); i++)
			btDeviceDiscoveredMain(btPairedDevices[i], true);
#else
		report_info("starting BT/BLE discovery");
		discoveryAgent->start();
		for (int i = 0; i < btPairedDevices.length(); i++)
			report_info("Paired = %s %s", qPrintable( btPairedDevices[i].name), qPrintable(btPairedDevices[i].address));
#endif

#if defined(Q_OS_IOS) || (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID))
		QTimer timer;
		timer.setSingleShot(true);
		connect(&timer, &QTimer::timeout, discoveryAgent, &QBluetoothDeviceDiscoveryAgent::stop);
		timer.start(3000);
#endif
	} else {
		report_info("localBtDevice isn't valid or not connectable");
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
	report_info("BT/BLE finished discovery");
	QList<QBluetoothDeviceInfo> devList = discoveryAgent->discoveredDevices();
	for (QBluetoothDeviceInfo device: devList) {
		report_info("%s %s", qPrintable(device.name()), qPrintable(device.address().toString()));
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
		report_info("%s", qPrintable(id.toByteArray()));
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

	dc_transport_t transport = device.address.startsWith("LE:") ?
		DC_TRANSPORT_BLE : DC_TRANSPORT_BLUETOOTH;

	QString newDevice;
	dc_descriptor_t *newDC = getDeviceType(device.name, transport);
	if (newDC)
		newDevice = dc_descriptor_get_product(newDC);
	else
		newDevice = device.name;

	QString msg;
	msg = QString("%1 device: '%2' [%3]: ").arg(fromPaired ? "Paired" : "Discovered new").arg(newDevice).arg(device.address);
	if (newDC) {
		QString vendor = dc_descriptor_get_vendor(newDC);
		report_info("%s this could be a %s", qPrintable(msg), qPrintable(vendor));
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
	report_info("%s not recognized as dive computer", qPrintable(msg));
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
				report_info("paired BT classic device type %d with address %s", btType, qPrintable(result.address));
				btPairedDevices.append(result);
			}
			if (btType & 2) { // DEVICE_TYPE_LE
				result.address = QString("LE:%1").arg(result.address);
				report_info("paired BLE device type %d with address %s", btType, qPrintable(result.address));
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
		report_info("restarting discovery agent");
		discoveryAgent->start();
	}
}

void BTDiscovery::stopAgent()
{
	if (!discoveryAgent)
		return;
	report_info("---> stopping the discovery agent");
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

std::pair<QString, QString> extractBluetoothNameAddress(const QString &address)
{
	// sometimes our device text is of the form "name (address)", sometimes it's just "address"
	// let's simply return the address
	QString extractedAddress = extractBluetoothAddress(address);
	if (extractedAddress == address.trimmed())
		return { address, QString() };

	QRegularExpression re("^([^()]+)\\(([^)]*\\))$");
	QRegularExpressionMatch m = re.match(address);
	if (m.hasMatch()) {
		QString name = m.captured(1).trimmed();
		return { extractedAddress, name };
	}
	report_info("can't parse address %s", qPrintable(address));
	return { QString(), QString() };
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
		report_info("still looking scan is still running, we should just wait for a few moments");
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
	report_info("notify user that we can't find %s", qPrintable(devaddr));
	return QBluetoothDeviceInfo();
}
#endif // BT_SUPPORT
