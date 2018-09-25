// SPDX-License-Identifier: GPL-2.0
#ifndef BTDEVICESELECTIONDIALOG_H
#define BTDEVICESELECTIONDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include <QPointer>
#include <QtBluetooth/QBluetoothLocalDevice>
#include <QtBluetooth/QBluetoothDeviceDiscoveryAgent>
#include "core/libdivecomputer.h"

#if defined(Q_OS_WIN)
	#include <QThread>
	#include <QMutex>
	#include <QWaitCondition>
	#include <winsock2.h>
	#include <ws2bth.h>

	#define SUCCESS				0
	#define BTH_ADDR_BUF_LEN                40
	#define BTH_ADDR_PRETTY_STRING_LEN	17	// there are 6 two-digit hex values and 5 colons

	#undef ERROR				// this is already declared in our headers
	#undef IGNORE				// this is already declared in our headers
	#undef DC_VERSION			// this is already declared in libdivecomputer header
#endif

namespace Ui {
	class BtDeviceSelectionDialog;
}

#if defined(Q_OS_WIN)
class WinBluetoothDeviceDiscoveryAgent : public QThread {
	Q_OBJECT
signals:
	void deviceDiscovered(const QBluetoothDeviceInfo &info);
	void error(QBluetoothDeviceDiscoveryAgent::Error error);

public:
	WinBluetoothDeviceDiscoveryAgent(QObject *parent);
	~WinBluetoothDeviceDiscoveryAgent();
	bool isActive() const;
	QString errorToString() const;
	QBluetoothDeviceDiscoveryAgent::Error error() const;
	void run();
	void start();
	void stop();

private:
	void doWork();

	mutable QMutex lock;
	mutable QWaitCondition cond;
	bool stopped;			// If true, don't scan
	bool quit;			// If true, exit thread
	QString lastErrorToString;
	QBluetoothDeviceDiscoveryAgent::Error lastError;
	void reportError(QBluetoothDeviceDiscoveryAgent::Error errorCode);
};
#endif

class BtDeviceSelectionDialog : public QDialog {
	Q_OBJECT

public:
	explicit BtDeviceSelectionDialog(const QString &address, dc_descriptor_t *dc, QWidget *parent = 0);
	~BtDeviceSelectionDialog();
	QString getSelectedDeviceAddress();
	QString getSelectedDeviceName();
	QString getSelectedDeviceText();
	static QString formatDeviceText(const QString &address, const QString &name);

private slots:
	void on_changeDeviceState_clicked();
	void on_save_clicked();
	void on_clear_clicked();
	void on_scan_clicked();
	void on_quit_clicked();
	void remoteDeviceScanFinished();
	void hostModeStateChanged(QBluetoothLocalDevice::HostMode mode);
	void addRemoteDevice(const QBluetoothDeviceInfo &remoteDeviceInfo);
	void currentItemChanged(QListWidgetItem *item,QListWidgetItem *previous);
	void displayPairingMenu(const QPoint &pos);
	void pairingFinished(const QBluetoothAddress &address,QBluetoothLocalDevice::Pairing pairing);
	void error(QBluetoothLocalDevice::Error error);
	void deviceDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error error);
	void localDeviceChanged(int);

private:
	Ui::BtDeviceSelectionDialog *ui;
	QString previousDevice;
	dc_descriptor_t *dcDescriptor;
	int maxPriority;
#if defined(Q_OS_WIN)
	WinBluetoothDeviceDiscoveryAgent *remoteDeviceDiscoveryAgent;
#else
	QBluetoothLocalDevice *localDevice;
	QBluetoothDeviceDiscoveryAgent *remoteDeviceDiscoveryAgent;
#endif
	QScopedPointer<QBluetoothDeviceInfo> selectedRemoteDeviceInfo;

	void updateLocalDeviceInformation();
	void initializeDeviceDiscoveryAgent();
	bool isPoweredOn() const;
	void setScanStatusOn();
	void setScanStatusOff();
	void setScanStatusInvalid();
	void startScan();
	void stopScan();
	void showEvent(QShowEvent *event);
	void initializeDeviceList();
	int getDevicePriority(bool connectable, const QBluetoothDeviceInfo &remoteDeviceInfo);
	void saveDeviceList();
	void loadDeviceList();
};

#endif // BTDEVICESELECTIONDIALOG_H
