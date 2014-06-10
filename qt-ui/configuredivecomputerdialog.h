#ifndef CONFIGUREDIVECOMPUTERDIALOG_H
#define CONFIGUREDIVECOMPUTERDIALOG_H

#include <QDialog>
#include <QStringListModel>
#include "../libdivecomputer.h"
#include "configuredivecomputer.h"

namespace Ui {
class ConfigureDiveComputerDialog;
}

class ConfigureDiveComputerDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ConfigureDiveComputerDialog(QWidget *parent = 0);
	~ConfigureDiveComputerDialog();

private slots:
	void on_vendor_currentIndexChanged(const QString &vendor);

	void on_product_currentIndexChanged(const QString &product);

	void readSettings();
	void configMessage(QString msg);
	void configError(QString err);
	void on_cancel_clicked();
	void deviceReadFinished();
	void on_saveSettingsPushButton_clicked();
	void deviceDetailsReceived(DeviceDetails *newDeviceDetails);
	void reloadValues();
private:
	Ui::ConfigureDiveComputerDialog *ui;

	ConfigureDiveComputer *config;
	device_data_t device_data;
	void getDeviceData();

	QStringList vendorList;
	QHash<QString, QStringList> productList;
	QHash<QString, dc_descriptor_t *> descriptorLookup;

	QStringListModel *vendorModel;
	QStringListModel *productModel;
	void fill_computer_list();
	void fill_device_list(int dc_type);

	DeviceDetails *deviceDetails;
};

#endif // CONFIGUREDIVECOMPUTERDIALOG_H
