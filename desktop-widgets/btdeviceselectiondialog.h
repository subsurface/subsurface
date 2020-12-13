// SPDX-License-Identifier: GPL-2.0
#ifndef BTDEVICESELECTIONDIALOG_H
#define BTDEVICESELECTIONDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include <QtBluetooth/QBluetoothLocalDevice>
#include <QtBluetooth/QBluetoothDeviceDiscoveryAgent>

namespace Ui {
	class BtDeviceSelectionDialog;
}

class BtDeviceSelectionDialog : public QDialog {
	Q_OBJECT

public:
	explicit BtDeviceSelectionDialog(QWidget *parent = 0);
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
	QBluetoothLocalDevice *localDevice;
	QBluetoothDeviceDiscoveryAgent *remoteDeviceDiscoveryAgent;
	QScopedPointer<QBluetoothDeviceInfo> selectedRemoteDeviceInfo;

	void updateLocalDeviceInformation();
	void initializeDeviceDiscoveryAgent();
};

#endif // BTDEVICESELECTIONDIALOG_H
