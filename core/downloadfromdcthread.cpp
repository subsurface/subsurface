#include "downloadfromdcthread.h"
#include "core/errorhelper.h"
#include "core/format.h"
#include "core/libdivecomputer.h"
#include "core/qthelper.h"
#include "core/range.h"
#include "core/uemis.h"
#include "core/settings/qPrefDiveComputer.h"
#include "core/divelist.h"
#if defined(Q_OS_ANDROID)
#include "core/subsurface-string.h"
#endif

QStringList vendorList;
QHash<QString, QStringList> productList;
static QHash<QString, QStringList> mobileProductList; // BT, BLE or FTDI supported DCs for mobile
QMap<QString, dc_descriptor_t *> descriptorLookup;
ConnectionListModel connectionListModel;

static void updateRememberedDCs()
{
	QString current = qPrefDiveComputer::vendor() + " - " + qPrefDiveComputer::product() + " - " + qPrefDiveComputer::device();
	QStringList dcs = {
		qPrefDiveComputer::vendor1() + " - " + qPrefDiveComputer::product1() + " - " + qPrefDiveComputer::device1(),
		qPrefDiveComputer::vendor2() + " - " + qPrefDiveComputer::product2() + " - " + qPrefDiveComputer::device2(),
		qPrefDiveComputer::vendor3() + " - " + qPrefDiveComputer::product3() + " - " + qPrefDiveComputer::device3(),
		qPrefDiveComputer::vendor4() + " - " + qPrefDiveComputer::product4() + " - " + qPrefDiveComputer::device4()
	};
	if (dcs.contains(current))
		// already in the list
		return;
	// add the current one as the first remembered one and drop the 4th one
	// don't get confused by 0-based and 1-based indices!
	if (dcs[2] != " - ") {
		qPrefDiveComputer::set_vendor4(qPrefDiveComputer::vendor3());
		qPrefDiveComputer::set_product4(qPrefDiveComputer::product3());
		qPrefDiveComputer::set_device4(qPrefDiveComputer::device3());
	}
	if (dcs[1] != " - ") {
		qPrefDiveComputer::set_vendor3(qPrefDiveComputer::vendor2());
		qPrefDiveComputer::set_product3(qPrefDiveComputer::product2());
		qPrefDiveComputer::set_device3(qPrefDiveComputer::device2());
	}
	if (dcs[0] != " - ") {
		qPrefDiveComputer::set_vendor2(qPrefDiveComputer::vendor1());
		qPrefDiveComputer::set_product2(qPrefDiveComputer::product1());
		qPrefDiveComputer::set_device2(qPrefDiveComputer::device1());
	}
	qPrefDiveComputer::set_vendor1(qPrefDiveComputer::vendor());
	qPrefDiveComputer::set_product1(qPrefDiveComputer::product());
	qPrefDiveComputer::set_device1(qPrefDiveComputer::device());
}

static QString transportStringTable[] = {
	QStringLiteral("SERIAL"),
	QStringLiteral("USB"),
	QStringLiteral("USBHID"),
	QStringLiteral("IRDA"),
	QStringLiteral("BT"),
	QStringLiteral("BLE"),
	QStringLiteral("USBSTORAGE")
};

static QString getTransportString(unsigned int transport)
{
	QString ts;
	for (auto [i, s]: enumerated_range(transportStringTable)) {
		if (transport & 1 << i)
			ts += s + ", ";
	}
	ts.chop(2);
	return ts;
}

DownloadThread::DownloadThread() : m_data(DCDeviceData::instance())
{
}

void DownloadThread::run()
{
	auto internalData = m_data->internalData();
	internalData->descriptor = descriptorLookup[m_data->vendor().toLower() + m_data->product().toLower()];
	internalData->log = &log;
	internalData->btname = m_data->devBluetoothName().toStdString();
	if (!internalData->descriptor) {
		report_info("No download possible when DC type is unknown");
		return;
	}
	// get the list of transports that this device supports and filter depending on Bluetooth option
	unsigned int transports = dc_descriptor_get_transports(internalData->descriptor);
	if (internalData->bluetooth_mode)
		transports &= (DC_TRANSPORT_BLE | DC_TRANSPORT_BLUETOOTH);
	else
		transports &= ~(DC_TRANSPORT_BLE | DC_TRANSPORT_BLUETOOTH);
	if (transports == DC_TRANSPORT_USBHID)
		internalData->devname = "";

	report_info("Starting download from %s", qPrintable(getTransportString(transports)));
	report_info("downloading %s dives", internalData->force_download ? "all" : "only new");
	log.clear();

	Q_ASSERT(internalData->log != nullptr);
	std::string errorText;
	import_thread_cancelled = false;
	error.clear();
	successful = false;
	if (internalData->vendor == "Uemis")
		errorText = do_uemis_import(internalData);
	else
		errorText = do_libdivecomputer_import(internalData);
	if (!errorText.empty()) {
		error = format_string_std(errorText.c_str(), internalData->devname.c_str(),
					  internalData->vendor.c_str(), internalData->product.c_str());
		report_info("Finishing download thread: %s", error.c_str());
	} else {
		successful = true;
		if (log.dives.empty())
			error = tr("No new dives downloaded from dive computer").toStdString();
		report_info("Finishing download thread: %d dives downloaded", static_cast<int>(log.dives.size()));
	}
	qPrefDiveComputer::set_vendor(internalData->vendor.c_str());
	qPrefDiveComputer::set_product(internalData->product.c_str());
	qPrefDiveComputer::set_device(internalData->devname.c_str());
	qPrefDiveComputer::set_device_name(m_data->devBluetoothName());

	updateRememberedDCs();
}

void fill_computer_list()
{
	dc_iterator_t *iterator = NULL;
	dc_descriptor_t *descriptor = NULL;

	unsigned int transportMask = get_supported_transports(NULL);

	dc_descriptor_iterator(&iterator);
	while (dc_iterator_next(iterator, &descriptor) == DC_STATUS_SUCCESS) {
		// mask out the transports that aren't supported
		unsigned int transports = dc_descriptor_get_transports(descriptor) & transportMask;
		if (transports == 0)
			// none of the transports are available, skip
			continue;

		const char *vendor = dc_descriptor_get_vendor(descriptor);
		const char *product = dc_descriptor_get_product(descriptor);
		if (!vendorList.contains(vendor))
			vendorList.append(vendor);
		if (!productList[vendor].contains(product))
			productList[vendor].append(product);

		descriptorLookup[QString(vendor).toLower() + QString(product).toLower()] = descriptor;
	}
	dc_iterator_free(iterator);
	for (const QString &vendor: vendorList) {
		auto &l = productList[vendor];
		std::sort(l.begin(), l.end());
	}

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
	/* currently suppress the Uemis Zurich on Q_OS_ANDROID and Q_OS_IOS,
	 * as it is no BT device */

	/* and add the Uemis Zurich which we are handling internally
	  THIS IS A HACK as we magically have a data structure here that
	  happens to match a data structure that is internal to libdivecomputer;
	  this WILL BREAK if libdivecomputer changes the dc_descriptor struct...
	  eventually the UEMIS code needs to move into libdivecomputer, I guess */
	struct mydescriptor *mydescriptor = (struct mydescriptor *)malloc(sizeof(struct mydescriptor));
	mydescriptor->vendor = "Uemis";
	mydescriptor->product = "Zurich";
	mydescriptor->type = DC_FAMILY_NULL;
	mydescriptor->model = 0;
	mydescriptor->transports = DC_TRANSPORT_USBSTORAGE;

	if (!vendorList.contains("Uemis"))
		vendorList.append("Uemis");

	if (!productList["Uemis"].contains("Zurich"))
		productList["Uemis"].push_back("Zurich");

	// note: keys in the descriptorLookup are always lowercase
	descriptorLookup["uemiszurich"] = (dc_descriptor_t *)mydescriptor;
#endif

	std::sort(vendorList.begin(), vendorList.end());
}

void show_computer_list()
{
	unsigned int transportMask = get_supported_transports(NULL);
	report_info("Supported dive computers:");
	for (const QString &vendor: vendorList) {
		QString msg = vendor + ": ";
		for (const QString &product: productList[vendor]) {
			dc_descriptor_t *descriptor = descriptorLookup[vendor.toLower() + product.toLower()];
			unsigned int transport = dc_descriptor_get_transports(descriptor) & transportMask;
			QString transportString = getTransportString(transport);
			msg += product + " (" + transportString + "), ";
		}
		msg.chop(2);
		report_info("%s", qPrintable(msg));
	}
}

DCDeviceData::DCDeviceData()
{
	data.log = nullptr;
	data.diveid = 0;
#if defined(BT_SUPPORT)
	data.bluetooth_mode = true;
#else
	data.bluetooth_mode = false;
#endif
	data.force_download = false;
	data.libdc_dump = false;
#if defined(SUBSURFACE_MOBILE)
	data.libdc_log = true;
#else
	data.libdc_log = false;
#endif
#if defined(Q_OS_ANDROID)
	data.androidUsbDeviceDescriptor = nullptr;
#endif
	data.sync_time = false;
}

DCDeviceData *DCDeviceData::instance()
{
	static DCDeviceData self;
	return &self;
}

QStringList DCDeviceData::getProductListFromVendor(const QString &vendor)
{
	return productList[vendor];
}

int DCDeviceData::getMatchingAddress(const QString &, const QString &product)
{
	return connectionListModel.indexOf(product);
}

DCDeviceData *DownloadThread::data()
{
	return m_data;
}

QString DCDeviceData::vendor() const
{
	return QString::fromStdString(data.vendor);
}

QString DCDeviceData::product() const
{
	return QString::fromStdString(data.product);
}

QString DCDeviceData::devName() const
{
	return QString::fromStdString(data.devname);
}

QString DCDeviceData::devBluetoothName() const
{
	return m_devBluetoothName;
}

QString DCDeviceData::descriptor() const
{
	return QString();
}

bool DCDeviceData::bluetoothMode() const
{
	return data.bluetooth_mode;
}

bool DCDeviceData::forceDownload() const
{
	return data.force_download;
}

int DCDeviceData::diveId() const
{
	return data.diveid;
}

bool DCDeviceData::syncTime() const
{
	return data.sync_time;
}

void DCDeviceData::setVendor(const QString &vendor)
{
	data.vendor = vendor.toStdString();
}

void DCDeviceData::setProduct(const QString &product)
{
	data.product = product.toStdString();
}

void DCDeviceData::setDevName(const QString &devName)
{
	// This is a workaround for bug #1002. A string of the form "devicename (deviceaddress)"
	// or "deviceaddress (devicename)" may have found its way into the preferences.
	// Try to fetch the address from such a string
	// TODO: Remove this code in due course
	if (data.bluetooth_mode) {
		int idx1 = devName.indexOf('(');
		int idx2 = devName.lastIndexOf(')');
		if (idx1 >= 0 && idx2 >= 0 && idx2 > idx1) {
			QString front = devName.left(idx1).trimmed();
			QString back = devName.mid(idx1 + 1, idx2 - idx1 - 1);
			QString newDevName = back.indexOf(':') >= 0 ? back : front;
			qWarning() << "Found invalid bluetooth device" << devName << "corrected to" << newDevName << ".";
			data.devname = newDevName.toStdString();
			return;
		}
	}
	data.devname = devName.toStdString();
}

#if defined(Q_OS_ANDROID)
void DCDeviceData::setUsbDevice(const android_usb_serial_device_descriptor &usbDescriptor)
{
	androidUsbDescriptor = usbDescriptor;
	data.androidUsbDeviceDescriptor = &androidUsbDescriptor;
}
#endif

void DCDeviceData::setDevBluetoothName(const QString &name)
{
	m_devBluetoothName = name;
}

void DCDeviceData::setBluetoothMode(bool mode)
{
	data.bluetooth_mode = mode;
}

void DCDeviceData::setForceDownload(bool force)
{
	data.force_download = force;
}

void DCDeviceData::setDiveId(int diveId)
{
	data.diveid = diveId;
}

void DCDeviceData::setSyncTime(bool syncTime)
{
	data.sync_time = syncTime;
}

void DCDeviceData::setSaveDump(bool save)
{
	data.libdc_dump = save;
}

bool DCDeviceData::saveDump() const
{
	return data.libdc_dump;
}

void DCDeviceData::setSaveLog(bool saveLog)
{
	data.libdc_log = saveLog;
}

bool DCDeviceData::saveLog() const
{
	return data.libdc_log;
}

device_data_t *DCDeviceData::internalData()
{
	return &data;
}

int DCDeviceData::getDetectedVendorIndex()
{
#if defined(BT_SUPPORT)
	QList<BTDiscovery::btVendorProduct> btDCs = BTDiscovery::instance()->getBtDcs();

	// Pick the vendor of the first confirmed find of a DC (if any)
	if (!btDCs.isEmpty())
		return btDCs.first().vendorIdx;
#endif
	return -1;
}

int DCDeviceData::getDetectedProductIndex(const QString &currentVendorText)
{
#if defined(BT_SUPPORT)
	QList<BTDiscovery::btVendorProduct> btDCs = BTDiscovery::instance()->getBtDcs();

	// Display in the QML UI, the first found dive computer that is been
	// detected as a possible real dive computer (and not some other paired
	// BT device)
	if (!btDCs.isEmpty())
		return btDCs.first().productIdx;
#endif
	return -1;
}
