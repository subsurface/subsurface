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

#include <QFile>
#include <QNetworkReply>

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

#define OSTC4_VERSION_MAJOR_SHIFT 11
#define OSTC4_VERSION_MINOR_SHIFT 6
#define OSTC4_VERSION_PATCH_SHIFT 1
#define OSTC4_VERSION_MASK 0x001F
#define OSTC4_VERSION_BETA_MASK 0x0001

OstcFirmwareCheck::OstcFirmwareCheck(const QString &product)
{
	QUrl url;
	devData = device_data_t();
	if (product == "OSTC 3" || product == "OSTC 3+" || product == "OSTC cR" || product == "OSTC Plus") {
		url = QUrl("https://www.heinrichsweikamp.net/autofirmware/ostc3_changelog.txt");
		latestFirmwareHexFile = QUrl("https://www.heinrichsweikamp.net/autofirmware/ostc3_firmware.hex");
	} else if (product == "OSTC Sport") {
		url = QUrl("https://www.heinrichsweikamp.net/autofirmware/ostc_sport_changelog.txt");
		latestFirmwareHexFile = QUrl("https://www.heinrichsweikamp.net/autofirmware/ostc_sport_firmware.hex");
	} else if (product == "OSTC 4/5") {
		url = QUrl("https://www.heinrichsweikamp.net/autofirmware/ostc4_changelog.txt");
		latestFirmwareHexFile = QUrl("https://www.heinrichsweikamp.net/autofirmware/ostc4_firmware.bin");
	} else { // not one of the known dive computers
		return;
	}
	connect(&manager, &QNetworkAccessManager::finished, this, &OstcFirmwareCheck::parseOstcFwVersion);
	QNetworkRequest download(url);
	manager.get(download);
	report_info("Started download of latest OSTC firmware version information for %s.", qPrintable(product));
}

void OstcFirmwareCheck::parseOstcFwVersion(QNetworkReply *reply)
{
	QString parse = reply->readAll();
	int firstOpenBracket = parse.indexOf('[');
	int firstCloseBracket = parse.indexOf(']');
	latestFirmwareAvailable = parse.mid(firstOpenBracket + 1, firstCloseBracket - firstOpenBracket - 1);
	report_info("Found latest OSTC firmware version: %s.", qPrintable(latestFirmwareAvailable));

	reply->close();
	disconnect(&manager, &QNetworkAccessManager::finished, this, &OstcFirmwareCheck::parseOstcFwVersion);
}

bool OstcFirmwareCheck::checkLatest(device_data_t *data)
{
	devData = *data;
	// If we didn't find a current firmware version stop this whole thing here.
	if (latestFirmwareAvailable.isEmpty()) {
		emit checkCompleted();

		return true;
	}

	// libdivecomputer gives us the firmware on device as an integer
	// for the OSTC that means highbyte.lowbyte is the version number
	// For OSTC 4/5 it is stored as XXXX XYYY YYZZ ZZZB, -> X.Y.Z-beta?

	int firmwareOnDevice = devData.devinfo.firmware;
	// Convert the latestFirmwareAvailable to a integer we can compare with
	QStringList fwParts = latestFirmwareAvailable.split(".");

	bool canBeUpdated = false;
	if (data->product == "OSTC 4/5") {
		unsigned char first = (firmwareOnDevice >> OSTC4_VERSION_MAJOR_SHIFT) & OSTC4_VERSION_MASK;
		unsigned char second = (firmwareOnDevice >> OSTC4_VERSION_MINOR_SHIFT) & OSTC4_VERSION_MASK;
		unsigned char third = (firmwareOnDevice >> OSTC4_VERSION_PATCH_SHIFT) & OSTC4_VERSION_MASK;
		bool beta = firmwareOnDevice & OSTC4_VERSION_BETA_MASK;
		firmwareOnDeviceString = QString("%1.%2.%3%4").arg(first).arg(second).arg(third).arg(beta ? "-beta" : "");
		int latestFirmwareAvailableNumber = (fwParts[0].toInt() << OSTC4_VERSION_MAJOR_SHIFT) + (fwParts[1].toInt() << OSTC4_VERSION_MINOR_SHIFT) + (fwParts[2].toInt() << OSTC4_VERSION_PATCH_SHIFT);
		int firmwareOnDeviceWithoutBeta = firmwareOnDevice & ~OSTC4_VERSION_BETA_MASK;
		if (firmwareOnDeviceWithoutBeta < latestFirmwareAvailableNumber || (firmwareOnDeviceWithoutBeta == latestFirmwareAvailableNumber && beta))
			canBeUpdated = true;
	} else { // OSTC 3, Sport, Cr
		firmwareOnDeviceString = QString("%1.%2").arg(firmwareOnDevice / 256).arg(firmwareOnDevice % 256);
		int latestFirmwareAvailableNumber = fwParts[0].toInt() * 256 + fwParts[1].toInt();
		if (firmwareOnDevice < latestFirmwareAvailableNumber)
			canBeUpdated = true;
	}

	if (canBeUpdated)
		report_info("Found %s that can be updated from %s to %s.", data->product.c_str(), qPrintable(firmwareOnDeviceString), qPrintable(latestFirmwareAvailable));

	return !canBeUpdated;
}

QString OstcFirmwareCheck::getLatestFirmwareFileName()
{
	QString fileName = latestFirmwareHexFile.fileName();
	fileName.replace("firmware", latestFirmwareAvailable);

	return fileName;
}

QString OstcFirmwareCheck::getLatestFirmwareAvailable()
{
	return latestFirmwareAvailable;
}

QString OstcFirmwareCheck::getFirmwareOnDevice()
{
	return firmwareOnDeviceString;
}

void OstcFirmwareCheck::startFirmwareUpdate(const QString &filename, ConfigureDiveComputer *configureDiveComputer)
{
	storeFirmware = filename;
	config = configureDiveComputer;

	// start download of latestFirmwareHexFile
	connect(&manager, &QNetworkAccessManager::finished, this, &OstcFirmwareCheck::saveOstcFirmware);
	QNetworkRequest download(latestFirmwareHexFile);
	manager.get(download);
	report_info("Started download of latest OSTC firmware %s to %s.", qPrintable(latestFirmwareAvailable), qPrintable(storeFirmware));
}

void OstcFirmwareCheck::saveOstcFirmware(QNetworkReply *reply)
{
	QByteArray firmwareData = reply->readAll();
	QFile file(storeFirmware);
	file.open(QIODevice::WriteOnly);
	file.write(firmwareData);
	file.close();
	report_info("Downloaded OSTC firmware %s to %s.", qPrintable(latestFirmwareAvailable), qPrintable(storeFirmware));

	reply->close();
	emit checkCompleted();

	config->dc_open(&devData);
	config->startFirmwareUpdate(storeFirmware, &devData, false);
}

OstcFirmwareCheck *getOstcFirmwareCheck(const QString &product)
{
	if (product == "OSTC 3" || product == "OSTC 3+" || product == "OSTC cR" ||
		product == "OSTC Sport" || product == "OSTC 4/5" || product == "OSTC Plus") {
				return new OstcFirmwareCheck(product);
		}

	return nullptr;
}
