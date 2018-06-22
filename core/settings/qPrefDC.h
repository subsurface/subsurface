// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFSDC_H
#define QPREFSDC_H

#include <QObject>


class qPrefDC : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString	device
				READ	device
				WRITE	setDevice
				NOTIFY	deviceChanged);
	Q_PROPERTY(QString	device_name
				READ	deviceName
				WRITE	setDeviceName
				NOTIFY	deviceNameChanged);
	Q_PROPERTY(int		download_mode
				READ	downloadMode
				WRITE	setDownloadMode
				NOTIFY	downloadModeChanged);
	Q_PROPERTY(QString	product
				READ	product
				WRITE	setProduct
				NOTIFY	productChanged);
	Q_PROPERTY(QString	vendor
				READ	vendor
				WRITE	setVendor
				NOTIFY	vendorChanged);

public:
	qPrefDC(QObject *parent = NULL) : QObject(parent) {};
	~qPrefDC() {};
	static qPrefDC *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);

public:
	const QString device() const;
	const QString deviceName() const;
	int downloadMode() const;
	const QString product() const;
	const QString vendor() const;

public slots:
	void setDevice(const QString& device);
	void setDeviceName(const QString& device_name);
	void setDownloadMode(int mode);
	void setProduct(const QString& product);
	void setVendor(const QString& vendor);

signals:
	void deviceChanged(const QString& device);
	void deviceNameChanged(const QString& device_name);
	void downloadModeChanged(int mode);
	void productChanged(const QString& product);
	void vendorChanged(const QString& vendor);

private:
	const QString group = QStringLiteral("DiveComputer");
	static qPrefDC *m_instance;

	// functions to load/sync variable with disk
	void diskDevice(bool doSync);
	void diskDeviceName(bool doSync);
	void diskDownloadMode(bool doSync);
	void diskProduct(bool doSync);
	void diskVendor(bool doSync);
};

#endif
