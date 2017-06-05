#include "downloadfromdcthread.h"
#include "core/libdivecomputer.h"
#include <QDebug>

QStringList vendorList;
QHash<QString, QStringList> productList;
QMap<QString, dc_descriptor_t *> descriptorLookup;

static QString str_error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	const QString str = QString().vsprintf(fmt, args);
	va_end(args);

	return str;
}

DownloadThread::DownloadThread() : m_data(new DCDeviceData())
{
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

void fill_computer_list()
{
	dc_iterator_t *iterator = NULL;
	dc_descriptor_t *descriptor = NULL;
	struct mydescriptor *mydescriptor;

	QStringList computer;
	dc_descriptor_iterator(&iterator);
	while (dc_iterator_next(iterator, &descriptor) == DC_STATUS_SUCCESS) {
		const char *vendor = dc_descriptor_get_vendor(descriptor);
		const char *product = dc_descriptor_get_product(descriptor);

		if (!vendorList.contains(vendor))
			vendorList.append(vendor);

		if (!productList[vendor].contains(product))
			productList[vendor].push_back(product);

		descriptorLookup[QString(vendor) + QString(product)] = descriptor;
	}
	dc_iterator_free(iterator);
	Q_FOREACH (QString vendor, vendorList)
		qSort(productList[vendor]);

	/* and add the Uemis Zurich which we are handling internally
	  THIS IS A HACK as we magically have a data structure here that
	  happens to match a data structure that is internal to libdivecomputer;
	  this WILL BREAK if libdivecomputer changes the dc_descriptor struct...
	  eventually the UEMIS code needs to move into libdivecomputer, I guess */

	mydescriptor = (struct mydescriptor *)malloc(sizeof(struct mydescriptor));
	mydescriptor->vendor = "Uemis";
	mydescriptor->product = "Zurich";
	mydescriptor->type = DC_FAMILY_NULL;
	mydescriptor->model = 0;

	if (!vendorList.contains("Uemis"))
		vendorList.append("Uemis");

	if (!productList["Uemis"].contains("Zurich"))
		productList["Uemis"].push_back("Zurich");

	descriptorLookup["UemisZurich"] = (dc_descriptor_t *)mydescriptor;

	qSort(vendorList);
#if defined(SUBSURFACE_MOBILE) && defined(BT_SUPPORT)
	vendorList.append(QObject::tr("Paired Bluetooth Devices"));
#endif
}

DCDeviceData::DCDeviceData(QObject *parent) : QObject(parent)
{
	memset(&data, 0, sizeof(data));
	data.trip = nullptr;
	data.download_table = nullptr;
	data.diveid = 0;
	data.deviceid = 0;
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
