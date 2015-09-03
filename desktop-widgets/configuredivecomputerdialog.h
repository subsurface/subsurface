#ifndef CONFIGUREDIVECOMPUTERDIALOG_H
#define CONFIGUREDIVECOMPUTERDIALOG_H

#include <QDialog>
#include <QStringListModel>
#include "ui_configuredivecomputerdialog.h"
#include "subsurface-core/libdivecomputer.h"
#include "configuredivecomputer.h"
#include <QStyledItemDelegate>
#include <QNetworkAccessManager>
#ifdef BT_SUPPORT
#include "btdeviceselectiondialog.h"
#endif

class GasSpinBoxItemDelegate : public QStyledItemDelegate {
	Q_OBJECT

public:
	enum column_type {
		PERCENT,
		DEPTH,
		SETPOINT,
	};

	GasSpinBoxItemDelegate(QObject *parent = 0, column_type type = PERCENT);
	~GasSpinBoxItemDelegate();

	virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;
	virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

private:
	column_type type;
};

class GasTypeComboBoxItemDelegate : public QStyledItemDelegate {
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

class ConfigureDiveComputerDialog : public QDialog {
	Q_OBJECT

public:
	explicit ConfigureDiveComputerDialog(QWidget *parent = 0);
	~ConfigureDiveComputerDialog();

protected:
	void closeEvent(QCloseEvent *event);

private
slots:
	void checkLogFile(int state);
	void pickLogFile();
	void readSettings();
	void resetSettings();
	void configMessage(QString msg);
	void configError(QString err);
	void on_cancel_clicked();
	void on_saveSettingsPushButton_clicked();
	void deviceDetailsReceived(DeviceDetails *newDeviceDetails);
	void reloadValues();
	void on_backupButton_clicked();

	void on_restoreBackupButton_clicked();


	void on_updateFirmwareButton_clicked();

	void on_DiveComputerList_currentRowChanged(int currentRow);

	void dc_open();
	void dc_close();

#ifdef BT_SUPPORT
	void bluetoothSelectionDialogIsFinished(int result);
	void selectRemoteBluetoothDevice();
#endif

private:
	Ui::ConfigureDiveComputerDialog ui;

	QString logFile;

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
	bool fw_upgrade_possible;

#ifdef BT_SUPPORT
	BtDeviceSelectionDialog *btDeviceSelectionDialog;
#endif
};

class OstcFirmwareCheck : QObject {
	Q_OBJECT
public:
	explicit OstcFirmwareCheck(QString product);
	void checkLatest(QWidget *parent, device_data_t *data);
public
slots:
	void parseOstcFwVersion(QNetworkReply *reply);
	void saveOstcFirmware(QNetworkReply *reply);

private:
	void upgradeFirmware();
	device_data_t devData;
	QString latestFirmwareAvailable;
	QString latestFirmwareHexFile;
	QString storeFirmware;
	QWidget *parent;
	QNetworkAccessManager manager;
};

#endif // CONFIGUREDIVECOMPUTERDIALOG_H
