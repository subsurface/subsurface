#ifndef CONFIGUREDIVECOMPUTERDIALOG_H
#define CONFIGUREDIVECOMPUTERDIALOG_H

#include <QDialog>
#include <QStringListModel>
#include "ui_configuredivecomputerdialog.h"
#include "../libdivecomputer.h"
#include "configuredivecomputer.h"
#include <QStyledItemDelegate>

class GasSpinBoxItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	enum column_type {
		PERCENT,
		DEPTH,
	};

	GasSpinBoxItemDelegate(QObject *parent = 0, column_type type = PERCENT);
	~GasSpinBoxItemDelegate();

	virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;
	virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
private:
	column_type type;
};

class GasTypeComboBoxItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	enum computer_type {
		OSTC3,
		OSTC,
	};

	GasTypeComboBoxItemDelegate(QObject *parent = 0, computer_type type = OSTC3);
	~GasTypeComboBoxItemDelegate();

	virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;
	virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
private:
	computer_type type;
};

class ConfigureDiveComputerDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ConfigureDiveComputerDialog(QWidget *parent = 0);
	~ConfigureDiveComputerDialog();

private slots:
	void readSettings();
	void resetSettings();
	void configMessage(QString msg);
	void configError(QString err);
	void on_cancel_clicked();
	void deviceReadFinished();
	void on_saveSettingsPushButton_clicked();
	void deviceDetailsReceived(DeviceDetails *newDeviceDetails);
	void reloadValues();
	void on_backupButton_clicked();

	void on_restoreBackupButton_clicked();


	void on_updateFirmwareButton_clicked();

	void on_DiveComputerList_currentRowChanged(int currentRow);

private:
	Ui::ConfigureDiveComputerDialog ui;

	QStringList vendorList;
	QHash<QString, QStringList> productList;

	ConfigureDiveComputer *config;
	device_data_t device_data;
	void getDeviceData();

	QHash<QString, dc_descriptor_t *> descriptorLookup;
	void fill_device_list(int dc_type);
	void fill_computer_list();

	DeviceDetails *deviceDetails;
	void populateDeviceDetails();
	void populateDeviceDetailsOSTC3();
	void populateDeviceDetailsOSTC();
	void populateDeviceDetailsSuuntoVyper();
	void reloadValuesOSTC3();
	void reloadValuesOSTC();
	void reloadValuesSuuntoVyper();

	QString selected_vendor;
	QString selected_product;
};

#endif // CONFIGUREDIVECOMPUTERDIALOG_H
