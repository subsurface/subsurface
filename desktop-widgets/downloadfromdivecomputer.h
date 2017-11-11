// SPDX-License-Identifier: GPL-2.0
#ifndef DOWNLOADFROMDIVECOMPUTER_H
#define DOWNLOADFROMDIVECOMPUTER_H

#include <QDialog>
#include <QThread>
#include <QHash>
#include <QMap>
#include <QAbstractTableModel>

#include "core/libdivecomputer.h"
#include "desktop-widgets/configuredivecomputerdialog.h"
#include "core/downloadfromdcthread.h"

#include "ui_downloadfromdivecomputer.h"

#if defined(BT_SUPPORT)
#include "btdeviceselectiondialog.h"
#endif

class QStringListModel;
class DiveImportedModel;

class DownloadFromDCWidget : public QDialog {
	Q_OBJECT
public:
	explicit DownloadFromDCWidget(QWidget *parent = 0, Qt::WindowFlags f = 0);
	void reject();

	enum states {
		INITIAL,
		DOWNLOADING,
		CANCELLING,
		ERROR,
		DONE,
	};

public
slots:
	void on_downloadCancelRetryButton_clicked();
	void on_ok_clicked();
	void on_cancel_clicked();
	void on_search_clicked();
	void on_vendor_currentIndexChanged(const QString &vendor);
	void on_product_currentIndexChanged(const QString &product);

	void onDownloadThreadFinished();
	void updateProgressBar();
	void checkLogFile(int state);
	void checkDumpFile(int state);
	void pickDumpFile();
	void pickLogFile();
#if defined(BT_SUPPORT)
	void enableBluetoothMode(int state);
	void selectRemoteBluetoothDevice();
	void bluetoothSelectionDialogIsFinished(int result);
#endif
private:
	void markChildrenAsDisabled();
	void markChildrenAsEnabled();
	void updateDeviceEnabled();

	QStringListModel vendorModel;
	QStringListModel productModel;
	Ui::DownloadFromDiveComputer ui;
	DownloadThread thread;
	bool downloading;

	int previousLast;

	void fill_device_list(int dc_type);
	QTimer *timer;
	bool dumpWarningShown;
	OstcFirmwareCheck *ostcFirmwareCheck;
	DiveImportedModel *diveImportedModel;
#if defined(BT_SUPPORT)
	BtDeviceSelectionDialog *btDeviceSelectionDialog;
#endif

public:
	bool preferDownloaded();
	void updateState(states state);
	states currentState;
};

#endif // DOWNLOADFROMDIVECOMPUTER_H
