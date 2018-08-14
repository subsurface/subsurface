#include "downloadfromdcthread.h"
#include "core/libdivecomputer.h"
#include "core/qthelper.h"
#include "core/subsurface-qt/SettingsObjectWrapper.h"
#include "core/subsurface-string.h"
#include <QDebug>
#include <QRegularExpression>
#if defined(Q_OS_ANDROID)
#include "core/subsurface-string.h"
#endif

QStringList vendorList;
QHash<QString, QStringList> productList;
static QHash<QString, QStringList> mobileProductList; // BT, BLE or FTDI supported DCs for mobile
QMap<QString, dc_descriptor_t *> descriptorLookup;
ConnectionListModel connectionListModel;

static QString str_error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	const QString str = QString().vsprintf(fmt, args);
	va_end(args);

	return str;
}

DownloadThread::DownloadThread()
{
	m_data = DCDeviceData::instance();
}

void DownloadThread::run()
{
	auto internalData = m_data->internalData();
	internalData->descriptor = descriptorLookup[m_data->vendor() + m_data->product()];
	internalData->download_table = &downloadTable;
#if defined(Q_OS_ANDROID)
	// on Android we either use BT, a USB device, or we download via FTDI cable
	if (!internalData->bluetooth_mode && (same_string(internalData->devname, "FTDI") || same_string(internalData->devname, "")))
		internalData->devname = "ftdi";
#endif
	qDebug() << "Starting download from " << (internalData->bluetooth_mode ? "BT" : internalData->devname);
	downloadTable.nr = 0;
	dive_table.preexisting = dive_table.nr;

	Q_ASSERT(internalData->download_table != nullptr);
	const char *errorText;
	import_thread_cancelled = false;
	error.clear();
	if (!strcmp(internalData->vendor, "Uemis"))
		errorText = do_uemis_import(internalData);
	else
		errorText = do_libdivecomputer_import(internalData);
	if (errorText) {
		error = str_error(errorText, internalData->devname, internalData->vendor, internalData->product);
		qDebug() << "Finishing download thread:" << error;
	} else {
		qDebug() << "Finishing download thread:" << downloadTable.nr << "dives downloaded";
	}
	auto dcs = SettingsObjectWrapper::instance()->dive_computer_settings;
	dcs->set_vendor(internalData->vendor);
	dcs->set_product(internalData->product);
	dcs->set_device(internalData->devname);
	dcs->set_device_name(m_data->devBluetoothName());
}

static const QVector<dc_family_t> ftdiFamilies = {
	DC_FAMILY_SUUNTO_SOLUTION, DC_FAMILY_SUUNTO_EON, DC_FAMILY_SUUNTO_VYPER, DC_FAMILY_SUUNTO_VYPER2, DC_FAMILY_SUUNTO_D9,
	DC_FAMILY_OCEANIC_VTPRO, DC_FAMILY_OCEANIC_VEO250, DC_FAMILY_OCEANIC_ATOM2,
	DC_FAMILY_HW_OSTC, DC_FAMILY_HW_OSTC3, // but careful, the OSTC 4 is DC_FAMILY_HW_OSTC3 but has no serial connector
	DC_FAMILY_CRESSI_EDY, DC_FAMILY_CRESSI_LEONARDO,
	DC_FAMILY_DIVERITE_NITEKQ,
	DC_FAMILY_DIVESYSTEM_IDIVE,
	DC_FAMILY_TECDIVING_DIVECOMPUTEREU};

bool guessIfFTDI(dc_descriptor_t *descriptor)
{
	if (!descriptor)
		return false;
	dc_family_t family = dc_descriptor_get_type(descriptor);
	if (family == DC_FAMILY_HW_OSTC3 && same_string(dc_descriptor_get_product(descriptor), "OSTC 4"))
		return false;
	if (ftdiFamilies.contains(family))
		return true;
	return false;
}

static void fill_supported_mobile_list()
{
#if defined(Q_OS_ANDROID)
	/* USB or FTDI devices that are supported on Android - this does NOT include the BLE or BT only devices */
	mobileProductList["Aeris"] =
		QStringList({{"500 AI"}, {"A300"}, {"A300 AI"}, {"A300CS"}, {"Atmos 2"}, {"Atmos AI"}, {"Atmos AI 2"}, {"Compumask"}, {"Elite"}, {"Elite T3"}, {"Epic"}, {"F10"}, {"F11"}, {"Manta"}, {"XR-1 NX"}, {"XR-2"}});
	mobileProductList["Aqualung"] =
		QStringList({{"i200"}, {"i300"}, {"i450T"}, {"i550"}, {"i750TC"}});
	mobileProductList["Beuchat"] =
		QStringList({{"Mundial 2"}, {"Mundial 3"}, {"Voyager 2G"}});
	mobileProductList["Cochran"] =
		QStringList({{"Commander I"}, {"Commander II"}, {"Commander TM"}, {"EMC-14"}, {"EMC-16"}, {"EMC-20H"}});
	mobileProductList["Cressi"] =
		QStringList({{"Leonardo"}, {"Giotto"}, {"Newton"}, {"Drake"}});
	mobileProductList["Genesis"] =
		QStringList({{"React Pro"}, {"React Pro White"}});
	mobileProductList["Heinrichs Weikamp"] =
		QStringList({{"Frog"}, {"OSTC"}, {"OSTC 2"}, {"OSTC 2C"}, {"OSTC 2N"}, {"OSTC 3"}, {"OSTC 3+"}, {"OSTC 4"}, {"OSTC Mk2"}, {"OSTC Plus"}, {"OSTC Sport"}, {"OSTC cR"}, {"OSTC 2 TR"}});
	mobileProductList["Hollis"] =
		QStringList({{"DG02"}, {"DG03"}, {"TX1"}});
	mobileProductList["Mares"] =
		QStringList({{"Puck Pro"}, {"Smart"}, {"Quad"}});
	mobileProductList["Oceanic"] =
		QStringList({{"Atom 1.0"}, {"Atom 2.0"}, {"Atom 3.0"}, {"Atom 3.1"}, {"Datamask"}, {"F10"}, {"F11"}, {"Geo"}, {"Geo 2.0"}, {"OC1"}, {"OCS"}, {"OCi"}, {"Pro Plus 2"}, {"Pro Plus 2.1"}, {"Pro Plus 3"}, {"VT 4.1"}, {"VT Pro"}, {"VT3"}, {"VT4"}, {"VTX"}, {"Veo 1.0"}, {"Veo 180"}, {"Veo 2.0"}, {"Veo 200"}, {"Veo 250"}, {"Veo 3.0"}, {"Versa Pro"}});
	mobileProductList["Scubapro"] =
		QStringList({{"Aladin Square"}, {"G2"}});
	mobileProductList["Seemann"] =
		QStringList({{"XP5"}});
	mobileProductList["Sherwood"] =
		QStringList({{"Amphos"}, {"Amphos Air"}, {"Insight"}, {"Insight 2"}, {"Vision"}, {"Wisdom"}, {"Wisdom 2"}, {"Wisdom 3"}});
	mobileProductList["Subgear"] =
		QStringList({{"XP-Air"}});
	mobileProductList["Suunto"] =
		QStringList({{"Cobra"}, {"Cobra 2"}, {"Cobra 3"}, {"D3"}, {"D4"}, {"D4f"}, {"D4i"}, {"D6"}, {"D6i"}, {"D9"}, {"D9tx"}, {"DX"}, {"EON Core"}, {"EON Steel"}, {"Eon"}, {"Gekko"}, {"HelO2"}, {"Mosquito"}, {"Solution"}, {"Solution Alpha"}, {"Solution Nitrox"}, {"Spyder"}, {"Stinger"}, {"Vyper"}, {"Vyper 2"}, {"Vyper Air"}, {"Vyper Novo"}, {"Vytec"}, {"Zoop"}, {"Zoop Novo"}});
	mobileProductList["Tusa"] =
		QStringList({{"Element II (IQ-750)"}, {"Zen (IQ-900)"}, {"Zen Air (IQ-950)"}});
	mobileProductList["Uwatec"] =
		QStringList({{"Aladin Air Twin"}, {"Aladin Air Z"}, {"Aladin Air Z Nitrox"}, {"Aladin Air Z O2"}, {"Aladin Pro"}, {"Aladin Pro Ultra"}, {"Aladin Sport Plus"}, {"Memomouse"}});
	mobileProductList["Atomic Aquatics"] =
		QStringList({{"Cobalt"}, {"Cobalt 2"}});

#endif
}

void fill_computer_list()
{
	dc_iterator_t *iterator = NULL;
	dc_descriptor_t *descriptor = NULL;

	unsigned int transportMask = get_supported_transports(NULL);

	fill_supported_mobile_list();

	dc_descriptor_iterator(&iterator);
	while (dc_iterator_next(iterator, &descriptor) == DC_STATUS_SUCCESS) {
		// mask out the transports that aren't supported
		unsigned int transports = dc_descriptor_get_transports(descriptor) & transportMask;
		if (transports == 0)
			// none of the transports are available, skip
			continue;

		const char *vendor = dc_descriptor_get_vendor(descriptor);
		const char *product = dc_descriptor_get_product(descriptor);
#if defined(Q_OS_ANDROID)
		if ((transports & ~(DC_TRANSPORT_SERIAL | DC_TRANSPORT_USB | DC_TRANSPORT_USBHID)) == 0)
			// if the only available transports are serial/USB, then check against
			// the ones that we explicitly support on Android
			if (!mobileProductList.contains(vendor) || !mobileProductList[vendor].contains(product))
				continue;
#endif
		if (!vendorList.contains(vendor))
			vendorList.append(vendor);
		if (!productList[vendor].contains(product))
			productList[vendor].append(product);

		descriptorLookup[QString(vendor) + QString(product)] = descriptor;
	}
	dc_iterator_free(iterator);
	Q_FOREACH (QString vendor, vendorList)
		qSort(productList[vendor]);

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

	if (!vendorList.contains("Uemis"))
		vendorList.append("Uemis");

	if (!productList["Uemis"].contains("Zurich"))
		productList["Uemis"].push_back("Zurich");

	descriptorLookup["UemisZurich"] = (dc_descriptor_t *)mydescriptor;
#endif

	qSort(vendorList);
}

#define NUMTRANSPORTS 6
static QString transportStringTable[NUMTRANSPORTS] = {
	QStringLiteral("SERIAL"),
	QStringLiteral("USB"),
	QStringLiteral("USBHID"),
	QStringLiteral("IRDA"),
	QStringLiteral("BT"),
	QStringLiteral("BLE")
};

static QString getTransportString(unsigned int transport)
{
	QString ts;
	for (int i = 0; i < NUMTRANSPORTS; i++) {
		if (transport & 1 << i)
			ts += transportStringTable[i] + ", ";
	}
	ts.chop(2);
	return ts;
}

void show_computer_list()
{
	unsigned int transportMask = get_supported_transports(NULL);
	qDebug() << "Supported dive computers:";
	Q_FOREACH (QString vendor, vendorList) {
		QString msg = vendor + ": ";
		Q_FOREACH (QString product, productList[vendor]) {
			dc_descriptor_t *descriptor = descriptorLookup[vendor + product];
			unsigned int transport = dc_descriptor_get_transports(descriptor) & transportMask;
			QString transportString = getTransportString(transport);
			msg += product + " (" + transportString + "), ";
		}
		msg.chop(2);
		qDebug() << msg;
	}
}
DCDeviceData *DCDeviceData::m_instance = NULL;

DCDeviceData::DCDeviceData()
{
	memset(&data, 0, sizeof(data));
	data.trip = nullptr;
	data.download_table = nullptr;
	data.diveid = 0;
	data.deviceid = 0;
#if defined(BT_SUPPORT)
	data.bluetooth_mode = true;
#else
	data.bluetooth_mode = false;
#endif
	data.force_download = false;
	data.create_new_trip = false;
	data.libdc_dump = false;
#if defined(SUBSURFACE_MOBILE)
	data.libdc_log = true;
#else
	data.libdc_log = false;
#endif
	if (m_instance) {
		qDebug() << "already have an instance of DCDevieData";
		return;
	}
	m_instance = this;
}

DCDeviceData *DCDeviceData::instance()
{
	if (!m_instance)
		m_instance = new DCDeviceData;
	return m_instance;
}

QStringList DCDeviceData::getProductListFromVendor(const QString &vendor)
{
	return productList[vendor];
}

int DCDeviceData::getMatchingAddress(const QString &vendor, const QString &product)
{
	auto dcs = SettingsObjectWrapper::instance()->dive_computer_settings;
	if (dcs->vendor() == vendor &&
	    dcs->product() == product) {
		// we are trying to show the last dive computer selected
		for (int i = 0; i < connectionListModel.rowCount(); i++) {
			QString address = connectionListModel.address(i);
			if (address.contains(dcs->device()))
				return i;
		}
	}

	for (int i = 0; i < connectionListModel.rowCount(); i++) {
		QString address = connectionListModel.address(i);
		if (address.contains(product))
			return i;
	}
	return -1;
}

DCDeviceData *DownloadThread::data()
{
	return m_data;
}

QString DCDeviceData::vendor() const
{
	return data.vendor;
}

QString DCDeviceData::product() const
{
	return data.product;
}

QString DCDeviceData::devName() const
{
	return data.devname;
}

QString DCDeviceData::devBluetoothName() const
{
	return m_devBluetoothName;
}

QString DCDeviceData::descriptor() const
{
	return "";
}

bool DCDeviceData::bluetoothMode() const
{
	return data.bluetooth_mode;
}

bool DCDeviceData::forceDownload() const
{
	return data.force_download;
}

bool DCDeviceData::createNewTrip() const
{
	return data.create_new_trip;
}

int DCDeviceData::deviceId() const
{
	return data.deviceid;
}

int DCDeviceData::diveId() const
{
	return data.diveid;
}

void DCDeviceData::setVendor(const QString &vendor)
{
	data.vendor = copy_qstring(vendor);
}

void DCDeviceData::setProduct(const QString &product)
{
	data.product = copy_qstring(product);
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
			data.devname = copy_qstring(newDevName);
			return;
		}
	}
	data.devname = copy_qstring(devName);
}

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

void DCDeviceData::setCreateNewTrip(bool create)
{
	data.create_new_trip = create;
}

void DCDeviceData::setDeviceId(int deviceId)
{
	data.deviceid = deviceId;
}

void DCDeviceData::setDiveId(int diveId)
{
	data.diveid = diveId;
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
	auto dcs = SettingsObjectWrapper::instance()->dive_computer_settings;
	if (!dcs->vendor().isEmpty()) {
		// use the last one
		for (int i = 0; i < vendorList.length(); i++) {
			if (vendorList[i] == dcs->vendor())
				return i;
		}
	}

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
	auto dcs = SettingsObjectWrapper::instance()->dive_computer_settings;
	if (!dcs->vendor().isEmpty()) {
		if (dcs->vendor() == currentVendorText) {
			// we are trying to show the last dive computer selected
			for (int i = 0; i < productList[currentVendorText].length(); i++) {
				if (productList[currentVendorText][i] == dcs->product())
					return i;
			}
		}
	}

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
