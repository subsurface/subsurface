// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFSDIVECOMPUTER_H
#define QPREFSDIVECOMPUTER_H

#include <QObject>


class qPrefDiveComputer : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString device READ dc_device WRITE setDevice NOTIFY deviceChanged);
	Q_PROPERTY(QString device_name READ dc_device_name WRITE setDeviceName NOTIFY deviceNameChanged);
	Q_PROPERTY(int download_mode READ downloadMode WRITE setDownloadMode NOTIFY downloadModeChanged);
	Q_PROPERTY(QString product READ dc_product WRITE setProduct NOTIFY	productChanged);
	Q_PROPERTY(QString vendor READ dc_vendor WRITE	setVendor NOTIFY vendorChanged);

public:
	qPrefDiveComputer(QObject *parent = NULL) : QObject(parent) {};
	~qPrefDiveComputer() {};
	static qPrefDiveComputer *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);

public:
	const QString dc_device() const;
	const QString dc_device_name() const;
	int downloadMode() const;
	const QString dc_product() const;
	const QString dc_vendor() const;

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
	static qPrefDiveComputer *m_instance;

	// functions to load/sync variable with disk
	void diskDevice(bool doSync);
	void diskDeviceName(bool doSync);
	void diskDownloadMode(bool doSync);
	void diskProduct(bool doSync);
	void diskVendor(bool doSync);
};

#endif
