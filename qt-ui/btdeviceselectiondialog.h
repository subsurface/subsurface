#ifndef BTDEVICESELECTIONDIALOG_H
#define BTDEVICESELECTIONDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include <QPointer>
#include <QtBluetooth/QBluetoothLocalDevice>
#include <QtBluetooth/qbluetoothglobal.h>
#include <QtBluetooth/QBluetoothDeviceDiscoveryAgent>

#if QT_VERSION < 0x050500
Q_DECLARE_METATYPE(QBluetoothDeviceInfo)
#endif

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

private slots:
	void on_changeDeviceState_clicked();
	void on_save_clicked();
	void on_clear_clicked();
	void on_scan_clicked();
	void remoteDeviceScanFinished();
	void hostModeStateChanged(QBluetoothLocalDevice::HostMode mode);
	void addRemoteDevice(const QBluetoothDeviceInfo &remoteDeviceInfo);
	void itemClicked(QListWidgetItem *item);
	void displayPairingMenu(const QPoint &pos);
	void pairingFinished(const QBluetoothAddress &address,QBluetoothLocalDevice::Pairing pairing);
	void error(QBluetoothLocalDevice::Error error);
	void deviceDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error error);
	void localDeviceChanged(int);

private:
	Ui::BtDeviceSelectionDialog *ui;
	QBluetoothLocalDevice *localDevice;
	QBluetoothDeviceDiscoveryAgent *remoteDeviceDiscoveryAgent;
	QSharedPointer<QBluetoothDeviceInfo> selectedRemoteDeviceInfo;

	void updateLocalDeviceInformation();
	void initializeDeviceDiscoveryAgent();
};

#endif // BTDEVICESELECTIONDIALOG_H
