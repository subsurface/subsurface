// SPDX-License-Identifier: GPL-2.0
#ifndef DOWNLOADFROMDIVECOMPUTER_H
#define DOWNLOADFROMDIVECOMPUTER_H

#include <QDialog>
#include <QThread>
#include <QHash>
#include <QMap>
#include <QAbstractTableModel>
#include <memory>
#include <string>

#include "core/libdivecomputer.h"
#include "core/downloadfromdcthread.h"
#include "desktop-widgets/configuredivecomputerdialog.h"

#include "ui_downloadfromdivecomputer.h"

#if defined(BT_SUPPORT)
#include "btdeviceselectiondialog.h"
#endif

class QStringListModel;
class DiveImportedModel;
class BTDiscovery;

class DownloadFromDCWidget : public QDialog {
	Q_OBJECT
public:
	explicit DownloadFromDCWidget(const QString &filename, QWidget *parent = 0);
	void reject();

	enum states {
		INITIAL,
		DOWNLOADING,
		CANCELLING,
		ERRORED,
		DONE,
	};

public
slots:
	void on_downloadCancelRetryButton_clicked();
	void on_ok_clicked();
	void on_cancel_clicked();
	void on_search_clicked();
	void on_vendor_currentTextChanged(const QString &vendor);
	void on_product_currentTextChanged(const QString &product);
	void on_device_currentTextChanged(const QString &device);

	void onDownloadThreadFinished();
	void updateProgressBar();
	void checkLogFile(int state);
	void checkDumpFile(int state);
	void pickDumpFile();
	void pickLogFile();
	void DC1Clicked();
	void DC2Clicked();
	void DC3Clicked();
	void DC4Clicked();
	void DeleteDC1Clicked();
	void DeleteDC2Clicked();
	void DeleteDC3Clicked();
	void DeleteDC4Clicked();
	int deviceIndex(QString deviceText);

#if defined(BT_SUPPORT)
	void enableBluetoothMode(int state);
	void selectRemoteBluetoothDevice();
	void bluetoothSelectionDialogIsFinished(int result);
#endif
private:
	void markChildrenAsDisabled();
	void markChildrenAsEnabled();
	void updateTransportSelection(bool changeSelection);
	void showRememberedDCs();
	void checkShowError(states state);
	void showUpdateDialog();

	QStringListModel vendorModel;
	QStringListModel productModel;
	Ui::DownloadFromDiveComputer ui;
	QString filename;
	std::string last_text;
	bool downloading;

	int previousLast;

	void fill_device_list(unsigned int transport);
	QTimer *timer;
	bool dumpWarningShown;
	std::unique_ptr<OstcFirmwareCheck> ostcFirmwareCheck;
	DiveImportedModel *diveImportedModel;
#if defined(BT_SUPPORT)
	BtDeviceSelectionDialog *btDeviceSelectionDialog;
	BTDiscovery *btd;
#endif
	void setSyncDiveComputerTime(bool value);

public:
	bool preferDownloaded();
	void updateState(states state);
	states currentState;
};

#endif // DOWNLOADFROMDIVECOMPUTER_H
