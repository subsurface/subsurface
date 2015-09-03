#ifndef BTDEVICESELECTIONDIALOG_H
#define BTDEVICESELECTIONDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include <QPointer>
#include <QtBluetooth/QBluetoothLocalDevice>
#include <QtBluetooth/qbluetoothglobal.h>
#include <QtBluetooth/QBluetoothDeviceDiscoveryAgent>

#if defined(Q_OS_WIN)
	#include <QThread>
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
	virtual void run();
	virtual void stop();

private:
	bool running;
	bool stopped;
	QString lastErrorToString;
	QBluetoothDeviceDiscoveryAgent::Error lastError;
};
#endif

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
#if defined(Q_OS_WIN)
	WinBluetoothDeviceDiscoveryAgent *remoteDeviceDiscoveryAgent;
#else
	QBluetoothLocalDevice *localDevice;
	QBluetoothDeviceDiscoveryAgent *remoteDeviceDiscoveryAgent;
#endif
	QSharedPointer<QBluetoothDeviceInfo> selectedRemoteDeviceInfo;

	void updateLocalDeviceInformation();
	void initializeDeviceDiscoveryAgent();
};

#endif // BTDEVICESELECTIONDIALOG_H
