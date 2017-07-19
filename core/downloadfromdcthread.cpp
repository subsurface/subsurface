#include "downloadfromdcthread.h"
#include "core/libdivecomputer.h"
#include <QDebug>
#include <QRegularExpression>

QStringList vendorList;
QHash<QString, QStringList> productList;
static QHash<QString, QStringList> mobileProductList;	// BT, BLE or FTDI supported DCs for mobile
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
	internalData->download_table = 	&downloadTable;
#if defined(Q_OS_ANDROID)
	// on Android we either use BT or we download via FTDI cable
	if (!internalData->bluetooth_mode)
		internalData->devname = "ftdi";
#endif
	qDebug() << "Starting download from " << (internalData->bluetooth_mode ? "BT" : internalData->devname);
	downloadTable.nr = 0;
	qDebug() << "Starting the thread" << downloadTable.nr;
	dive_table.preexisting = dive_table.nr;

	Q_ASSERT(internalData->download_table != nullptr);
	const char *errorText;
	import_thread_cancelled = false;
	if (!strcmp(internalData->vendor, "Uemis"))
		errorText = do_uemis_import(internalData);
	else
		errorText = do_libdivecomputer_import(internalData);
	if (errorText)
		error = str_error(errorText, internalData->devname, internalData->vendor, internalData->product);

	qDebug() << "Finishing the thread" << errorText << "dives downloaded" << downloadTable.nr;
}

static void fill_supported_mobile_list()
{
	// This segment of the source is automatically generated
	// please edit scripts/dcTransport.pl , regenerated the code and copy it here

#if defined(Q_OS_ANDROID)
	/* BT, BLE and FTDI devices */
	mobileProductList["Aeris"] =
			QStringList({{"500 AI"}, {"A300"}, {"A300 AI"}, {"A300CS"}, {"Atmos 2"}, {"Atmos AI"}, {"Atmos AI 2"}, {"Compumask"}, {"Elite"}, {"Elite T3"}, {"Epic"}, {"F10"}, {"F11"}, {"Manta"}, {"XR-1 NX"}, {"XR-2"}});
	mobileProductList["Aqualung"] =
			QStringList({{"i300"}, {"i450T"}, {"i550"}, {"i750TC"}});
	mobileProductList["Beuchat"] =
			QStringList({{"Mundial 2"}, {"Mundial 3"}, {"Voyager 2G"}});
	mobileProductList["Genesis"] =
			QStringList({{"React Pro"}, {"React Pro White"}});
	mobileProductList["Heinrichs Weikamp"] =
			QStringList({{"Frog"}, {"OSTC"}, {"OSTC 2"}, {"OSTC 2C"}, {"OSTC 2N"}, {"OSTC 3"}, {"OSTC 3+"}, {"OSTC 4"}, {"OSTC Mk2"}, {"OSTC Sport"}, {"OSTC cR"}});
	mobileProductList["Hollis"] =
			QStringList({{"DG02"}, {"DG03"}, {"TX1"}});
	mobileProductList["Oceanic"] =
			QStringList({{"Atom 1.0"}, {"Atom 2.0"}, {"Atom 3.0"}, {"Atom 3.1"}, {"Datamask"}, {"F10"}, {"F11"}, {"Geo"}, {"Geo 2.0"}, {"OC1"}, {"OCS"}, {"OCi"}, {"Pro Plus 2"}, {"Pro Plus 2.1"}, {"Pro Plus 3"}, {"VT 4.1"}, {"VT Pro"}, {"VT3"}, {"VT4"}, {"VTX"}, {"Veo 1.0"}, {"Veo 180"}, {"Veo 2.0"}, {"Veo 200"}, {"Veo 250"}, {"Veo 3.0"}, {"Versa Pro"}});
	mobileProductList["Scubapro"] =
			QStringList({{"G2"}});
	mobileProductList["Seemann"] =
			QStringList({{"XP5"}});
	mobileProductList["Shearwater"] =
			QStringList({{"Nerd"}, {"Perdix"}, {"Perdix AI"}, {"Petrel"}, {"Petrel 2"}, {"Predator"}});
	mobileProductList["Sherwood"] =
			QStringList({{"Amphos"}, {"Amphos Air"}, {"Insight"}, {"Insight 2"}, {"Vision"}, {"Wisdom"}, {"Wisdom 2"}, {"Wisdom 3"}});
	mobileProductList["Subgear"] =
			QStringList({{"XP-Air"}});
	mobileProductList["Suunto"] =
			QStringList({{"Cobra"}, {"Cobra 2"}, {"Cobra 3"}, {"D3"}, {"D4"}, {"D4i"}, {"D6"}, {"D6i"}, {"D9"}, {"D9tx"}, {"DX"}, {"EON Steel"}, {"Eon"}, {"Gekko"}, {"HelO2"}, {"Mosquito"}, {"Solution"}, {"Solution Alpha"}, {"Solution Nitrox"}, {"Spyder"}, {"Stinger"}, {"Vyper"}, {"Vyper 2"}, {"Vyper Air"}, {"Vyper Novo"}, {"Vytec"}, {"Zoop"}, {"Zoop Novo"}});
	mobileProductList["Tusa"] =
			QStringList({{"Element II (IQ-750)"}, {"Zen (IQ-900)"}, {"Zen Air (IQ-950)"}});
	mobileProductList["Uwatec"] =
			QStringList({{"Aladin Air Twin"}, {"Aladin Air Z"}, {"Aladin Air Z Nitrox"}, {"Aladin Air Z O2"}, {"Aladin Pro"}, {"Aladin Pro Ultra"}, {"Aladin Sport Plus"}, {"Memomouse"}});

#endif
#if defined(Q_OS_IOS)
	/* BLE only, Qt does not support classic BT on iOS */
	mobileProductList["Scubapro"] =
			QStringList({{"G2"}});
	mobileProductList["Shearwater"] =
			QStringList({{"Petrel"}, {"Petrel 2"}, {"Perdix"}, {"Perdix AI"}});
	mobileProductList["Suunto"] =
			QStringList({{"EON Steel"}});

#endif
	// end of the automatically generated code
}

void fill_computer_list()
{
	dc_iterator_t *iterator = NULL;
	dc_descriptor_t *descriptor = NULL;

	fill_supported_mobile_list();

	dc_descriptor_iterator(&iterator);
	while (dc_iterator_next(iterator, &descriptor) == DC_STATUS_SUCCESS) {
		const char *vendor = dc_descriptor_get_vendor(descriptor);
		const char *product = dc_descriptor_get_product(descriptor);
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
		if (!mobileProductList.contains(vendor))
			continue;
#endif
		if (!vendorList.contains(vendor))
			vendorList.append(vendor);
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
		if (!mobileProductList[vendor].contains(product))
			continue;
#endif
		if (!productList[vendor].contains(product))
			productList[vendor].push_back(product);

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

DCDeviceData *DCDeviceData::m_instance = NULL;

DCDeviceData::DCDeviceData(QObject *parent) : QObject(parent)
{
	if (m_instance) {
		qDebug() << "already have an instance of DCDevieData";
		return;
	}
	m_instance = this;
	memset(&data, 0, sizeof(data));
	data.trip = nullptr;
	data.download_table = nullptr;
	data.diveid = 0;
	data.deviceid = 0;
}

DCDeviceData *DCDeviceData::instance()
{
	if (!m_instance)
		m_instance = new DCDeviceData();
	return m_instance;
}

QStringList DCDeviceData::getProductListFromVendor(const QString &vendor)
{
	return productList[vendor];
}

int DCDeviceData::getMatchingAddress(const QString &vendor, const QString &product)
{
	for (int i = 0; i < connectionListModel.rowCount(); i++) {
		QString address = connectionListModel.address(i);
		if (address.contains(product))
			return i;
	}
	return -1;
}

DCDeviceData * DownloadThread::data()
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

void DCDeviceData::setVendor(const QString& vendor)
{
	data.vendor = strdup(qPrintable(vendor));
}

void DCDeviceData::setProduct(const QString& product)
{
	data.product = strdup(qPrintable(product));
}

void DCDeviceData::setDevName(const QString& devName)
{
	data.devname = strdup(qPrintable(devName));
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

device_data_t* DCDeviceData::internalData()
{
	return &data;
}

int DCDeviceData::getDetectedVendorIndex(const QString &currentText)
{
#if defined(BT_SUPPORT)
	QList<BTDiscovery::btVendorProduct> btDCs = BTDiscovery::instance()->getBtDcs();

	// Pick the vendor of the first confirmed find of a DC (if any)
	if (!btDCs.isEmpty())
		return btDCs.first().vendorIdx;
#endif
	return -1;
}

int DCDeviceData::getDetectedProductIndex(const QString &currentVendorText,
					  const QString &currentProductText)
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

QString DCDeviceData::getDetectedDeviceAddress(const QString &currentVendorText,
					       const QString &currentProductText)
{
#if defined(BT_SUPPORT)
	// Pull the vendor from the found devices that are possible real dive computers
	// HACK: this assumes that dive computer names are unique across vendors
	//       and will only give you the first of multiple identically named dive computers
	QList<BTDiscovery::btVendorProduct> btDCs = BTDiscovery::instance()->getBtDcs();
	BTDiscovery::btVendorProduct btDC;
	Q_FOREACH(btDC, btDCs) {
		if (currentProductText.startsWith(dc_descriptor_get_product(btDC.dcDescriptor)))
			return btDC.btpdi.address;
	}
#endif
	return QStringLiteral("cannot determine address of dive computer");
}
