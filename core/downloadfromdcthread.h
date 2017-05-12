#ifndef DOWNLOADFROMDCTHREAD_H
#define DOWNLOADFROMDCTHREAD_H

#include <QThread>
#include <QMap>
#include <QHash>

#include "dive.h"
#include "libdivecomputer.h"

/* Helper object for access of Device Data in QML */
class DCDeviceData : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString vendor READ vendor WRITE setVendor)
	Q_PROPERTY(QString product READ product WRITE setProduct)
	Q_PROPERTY(bool bluetoothMode READ bluetoothMode WRITE setBluetoothMode)
	Q_PROPERTY(QString devName READ devName WRITE setDevName)
	Q_PROPERTY(QString descriptor READ descriptor WRITE setDescriptor)
	Q_PROPERTY(bool forceDownload READ forceDownload WRITE setForceDownload)
	Q_PROPERTY(bool createNewTrip READ createNewTrip WRITE setCreateNewTrip)
	Q_PROPERTY(int deviceId READ deviceId WRITE setDeviceId)
	Q_PROPERTY(int diveId READ diveId WRITE setDiveId)

public:
	DCDeviceData(QObject *parent = nullptr);

	QString vendor() const;
	QString product() const;
	QString devName() const;
	QString descriptor() const;
	bool bluetoothMode() const;
	bool forceDownload() const;
	bool createNewTrip() const;
	int deviceId() const;
	int diveId() const;

public slots:
	void setVendor(const QString& vendor);
	void setProduct(const QString& product);
	void setDevName(const QString& devName);
	void setDescriptor(const QString& descriptor);
	void setBluetoothMode(bool mode);
	void setForceDownload(bool force);
	void setCreateNewTrip(bool create);
	void setDeviceId(int deviceId);
	void setDiveId(int diveId);

private:
	device_data_t data;
};

class DownloadThread : public QThread {
	Q_OBJECT
public:
	DownloadThread(QObject *parent, device_data_t *data);
	void setDiveTable(struct dive_table *table);
	void run() override;

	QString error;

private:
	device_data_t *data;
};

struct product {
	const char *product;
	dc_descriptor_t *descriptor;
	struct product *next;
};

struct vendor {
	const char *vendor;
	struct product *productlist;
	struct vendor *next;
};

struct mydescriptor {
	const char *vendor;
	const char *product;
	dc_family_t type;
	unsigned int model;
};

/* This fills the vendor list QStringList and related members.
* this code needs to be reworked to be less ugly, but it will
* stay like this for now.
*/
void fill_computer_list();
extern 	QStringList vendorList;
extern QHash<QString, QStringList> productList;
extern QMap<QString, dc_descriptor_t *> descriptorLookup;

#endif
