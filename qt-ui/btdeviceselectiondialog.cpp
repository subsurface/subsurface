#include <QShortcut>
#include <QDebug>
#include <QMessageBox>
#include <QMenu>

#include "ui_btdeviceselectiondialog.h"
#include "btdeviceselectiondialog.h"

BtDeviceSelectionDialog::BtDeviceSelectionDialog(QWidget *parent) :
	QDialog(parent),
	localDevice(new QBluetoothLocalDevice),
	ui(new Ui::BtDeviceSelectionDialog)
{
	// Check if Bluetooth is available on this device
	if (!localDevice->isValid()) {
		QMessageBox::warning(this, tr("Warning"),
				     "This should never happen, please contact the Subsurface developers "
				     "and tell them that the Bluetooth download mode doesn't work.");
		return;
	}

	ui->setupUi(this);

	// Quit button callbacks
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), this, SLOT(reject()));
	connect(ui->quit, SIGNAL(clicked()), this, SLOT(reject()));

	// Disable the save button because there is no device selected
	ui->save->setEnabled(false);

	connect(ui->discoveredDevicesList, SIGNAL(itemActivated(QListWidgetItem*)),
		this, SLOT(itemActivated(QListWidgetItem*)));

	// Set UI information about the local device
	ui->deviceAddress->setText(localDevice->address().toString());
	ui->deviceName->setText(localDevice->name());

	connect(localDevice, SIGNAL(hostModeStateChanged(QBluetoothLocalDevice::HostMode)),
		this, SLOT(hostModeStateChanged(QBluetoothLocalDevice::HostMode)));

	// Initialize the state of the local device and activate/deactive the scan button
	hostModeStateChanged(localDevice->hostMode());

	// Intialize the discovery agent
	remoteDeviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent();

	connect(remoteDeviceDiscoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)),
		this, SLOT(addRemoteDevice(QBluetoothDeviceInfo)));
	connect(remoteDeviceDiscoveryAgent, SIGNAL(finished()),
		this, SLOT(remoteDeviceScanFinished()));

	// Add context menu for devices to be able to pair them
	ui->discoveredDevicesList->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->discoveredDevicesList, SIGNAL(customContextMenuRequested(QPoint)),
		this, SLOT(displayPairingMenu(QPoint)));
	connect(localDevice, SIGNAL(pairingFinished(QBluetoothAddress, QBluetoothLocalDevice::Pairing)),
		this, SLOT(pairingFinished(QBluetoothAddress, QBluetoothLocalDevice::Pairing)));

	connect(localDevice, SIGNAL(error(QBluetoothLocalDevice::Error)),
		this, SLOT(error(QBluetoothLocalDevice::Error)));
}

BtDeviceSelectionDialog::~BtDeviceSelectionDialog()
{
	delete ui;
}

void BtDeviceSelectionDialog::on_changeDeviceState_clicked()
{
	if (localDevice->hostMode() == QBluetoothLocalDevice::HostPoweredOff) {
		ui->dialogStatus->setText("Trying to turn on the local Bluetooth device...");
		localDevice->powerOn();
	} else {
		ui->dialogStatus->setText("Trying to turn off the local Bluetooth device...");
		localDevice->setHostMode(QBluetoothLocalDevice::HostPoweredOff);
	}
}

void BtDeviceSelectionDialog::on_save_clicked()
{
	// Get the selected device. There will be always a selected device if the save button is enabled.
	QListWidgetItem *currentItem = ui->discoveredDevicesList->currentItem();
	QBluetoothDeviceInfo remoteDeviceInfo = currentItem->data(Qt::UserRole).value<QBluetoothDeviceInfo>();

	// Save the selected device
	selectedRemoteDeviceInfo = QSharedPointer<QBluetoothDeviceInfo>(new QBluetoothDeviceInfo(remoteDeviceInfo));

	// Close the device selection dialog and set the result code to Accepted
	accept();
}

void BtDeviceSelectionDialog::on_clear_clicked()
{
	ui->dialogStatus->setText("Remote devices list was cleaned.");
	ui->discoveredDevicesList->clear();
	ui->save->setEnabled(false);
}

void BtDeviceSelectionDialog::on_scan_clicked()
{
	ui->dialogStatus->setText("Scanning for remote devices...");
	remoteDeviceDiscoveryAgent->start();
	ui->scan->setEnabled(false);
}

void BtDeviceSelectionDialog::remoteDeviceScanFinished()
{
	ui->dialogStatus->setText("Scanning finished.");
	ui->scan->setEnabled(true);

#if defined(Q_OS_ANDROID)
	// Check if there is a selected device and activate the Save button if it is paired
	QListWidgetItem *currentItem = ui->discoveredDevicesList->currentItem();

	if (currentItem != NULL) {
		QBluetoothDeviceInfo remoteDeviceInfo = currentItem->data(Qt::UserRole).value<QBluetoothDeviceInfo>();
		QBluetoothLocalDevice::Pairing pairingStatus = localDevice->pairingStatus(remoteDeviceInfo.address());

		if (pairingStatus != QBluetoothLocalDevice::Unpaired) {
			ui->save->setEnabled(true);
			ui->dialogStatus->setText("Scanning finished. You can press the Save button and start the download.");
		}
	}
#endif
}

void BtDeviceSelectionDialog::hostModeStateChanged(QBluetoothLocalDevice::HostMode mode)
{
	bool on = !(mode == QBluetoothLocalDevice::HostPoweredOff);

	ui->dialogStatus->setText(QString("The local Bluetooth device was turned %1.")
				  .arg(on? "ON" : "OFF"));
	ui->deviceState->setChecked(on);
	ui->scan->setEnabled(on);
}

void BtDeviceSelectionDialog::addRemoteDevice(const QBluetoothDeviceInfo &remoteDeviceInfo)
{
	QString deviceLable = QString("%1  (%2)").arg(remoteDeviceInfo.name()).arg(remoteDeviceInfo.address().toString());
	QList<QListWidgetItem *> itemsWithSameSignature = ui->discoveredDevicesList->findItems(deviceLable, Qt::MatchStartsWith);

	// Check if the remote device is already in the list
	if (itemsWithSameSignature.empty()) {
		QListWidgetItem *item = new QListWidgetItem(deviceLable);
		QBluetoothLocalDevice::Pairing pairingStatus = localDevice->pairingStatus(remoteDeviceInfo.address());
		item->setData(Qt::UserRole, QVariant::fromValue(remoteDeviceInfo));

		if (pairingStatus == QBluetoothLocalDevice::Paired) {
			item->setText(QString("%1   [State: PAIRED]").arg(item->text()));
			item->setBackgroundColor(QColor(Qt::gray));
		} else if (pairingStatus == QBluetoothLocalDevice::AuthorizedPaired) {
			item->setText(QString("%1   [State: AUTHORIZED_PAIRED]").arg(item->text()));
			item->setBackgroundColor(QColor(Qt::blue));
		} else {
			item->setText(QString("%1   [State: UNPAIRED]").arg(item->text()));
			item->setTextColor(QColor(Qt::black));
		}

		ui->discoveredDevicesList->addItem(item);
	}
}

void BtDeviceSelectionDialog::itemActivated(QListWidgetItem *item)
{
	QBluetoothDeviceInfo remoteDeviceInfo = item->data(Qt::UserRole).value<QBluetoothDeviceInfo>();
	QBluetoothLocalDevice::Pairing pairingStatus = localDevice->pairingStatus(remoteDeviceInfo.address());

	if (pairingStatus == QBluetoothLocalDevice::Unpaired) {
		ui->dialogStatus->setText(QString("The device %1 must be paired in order to be used. Please use the context menu for pairing options.")
					  .arg(remoteDeviceInfo.address().toString()));
		ui->save->setEnabled(false);
	} else {
#if defined(Q_OS_ANDROID)
		if (remoteDeviceDiscoveryAgent->isActive()) {
			ui->dialogStatus->setText(QString("The device %1 can be used for connection. Wait until the device scanning is done and press the Save button.")
						  .arg(remoteDeviceInfo.address().toString()));
			return;
		}
#endif
		ui->dialogStatus->setText(QString("The device %1 can be used for connection. You can press the Save button.")
					  .arg(remoteDeviceInfo.address().toString()));
		ui->save->setEnabled(true);
	}
}

void BtDeviceSelectionDialog::displayPairingMenu(const QPoint &pos)
{
	QMenu menu(this);
	QAction *pairAction = menu.addAction("Pair");
	QAction *removePairAction = menu.addAction("Remove Pairing");
	QAction *chosenAction = menu.exec(ui->discoveredDevicesList->viewport()->mapToGlobal(pos));
	QListWidgetItem *currentItem = ui->discoveredDevicesList->currentItem();
	QBluetoothDeviceInfo currentRemoteDeviceInfo = currentItem->data(Qt::UserRole).value<QBluetoothDeviceInfo>();
	QBluetoothLocalDevice::Pairing pairingStatus = localDevice->pairingStatus(currentRemoteDeviceInfo.address());

	//TODO: disable the actions
	if (pairingStatus == QBluetoothLocalDevice::Unpaired) {
		pairAction->setEnabled(true);
		removePairAction->setEnabled(false);
	} else {
		pairAction->setEnabled(false);
		removePairAction->setEnabled(true);
	}

	if (chosenAction == pairAction) {
		ui->dialogStatus->setText(QString("Trying to pair device %1")
					  .arg(currentRemoteDeviceInfo.address().toString()));
		localDevice->requestPairing(currentRemoteDeviceInfo.address(), QBluetoothLocalDevice::Paired);
	} else if (chosenAction == removePairAction) {
		ui->dialogStatus->setText(QString("Trying to unpair device %1")
					  .arg(currentRemoteDeviceInfo.address().toString()));
		localDevice->requestPairing(currentRemoteDeviceInfo.address(), QBluetoothLocalDevice::Unpaired);
	}
}

void BtDeviceSelectionDialog::pairingFinished(const QBluetoothAddress &address, QBluetoothLocalDevice::Pairing pairing)
{
	QString remoteDeviceStringAddress = address.toString();
	QList<QListWidgetItem *> items = ui->discoveredDevicesList->findItems(remoteDeviceStringAddress, Qt::MatchContains);

	if (pairing == QBluetoothLocalDevice::Paired || pairing == QBluetoothLocalDevice::Paired ) {
		ui->dialogStatus->setText(QString("Device %1 was paired.")
					  .arg(remoteDeviceStringAddress));

		for (int i = 0; i < items.count(); ++i) {
			QListWidgetItem *item = items.at(i);

			item->setText(QString("%1   [State: PAIRED]").arg(remoteDeviceStringAddress));
			item->setBackgroundColor(QColor(Qt::gray));
		}

		QListWidgetItem *currentItem = ui->discoveredDevicesList->currentItem();

		if (currentItem != NULL && currentItem->text().contains(remoteDeviceStringAddress, Qt::CaseInsensitive)) {
			ui->dialogStatus->setText(QString("The device %1 can now be used for connection. You can press the Save button.")
						  .arg(remoteDeviceStringAddress));
			ui->save->setEnabled(true);
		}
	} else {
		ui->dialogStatus->setText(QString("Device %1 was unpaired.")
					  .arg(remoteDeviceStringAddress));

		for (int i = 0; i < items.count(); ++i) {
			QListWidgetItem *item = items.at(i);

			item->setText(QString("%1   [State: UNPAIRED]").arg(remoteDeviceStringAddress));
			item->setBackgroundColor(QColor(Qt::white));
		}

		QListWidgetItem *currentItem = ui->discoveredDevicesList->currentItem();

		if (currentItem != NULL && currentItem->text().contains(remoteDeviceStringAddress, Qt::CaseInsensitive)) {
			ui->dialogStatus->setText(QString("The device %1 must be paired in order to be used. Please use the context menu for pairing options.")
						  .arg(remoteDeviceStringAddress));
			ui->save->setEnabled(false);
		}
	}
}

void BtDeviceSelectionDialog::error(QBluetoothLocalDevice::Error error)
{
	ui->dialogStatus->setText(QString("Local device error: %1.")
				  .arg((error == QBluetoothLocalDevice::PairingError)? "Pairing error" : "Unknown error"));
}

QString BtDeviceSelectionDialog::getSelectedDeviceAddress()
{
	if (selectedRemoteDeviceInfo) {
		return selectedRemoteDeviceInfo.data()->address().toString();
	}

	return QString();
}

QString BtDeviceSelectionDialog::getSelectedDeviceName()
{
	if (selectedRemoteDeviceInfo) {
		return selectedRemoteDeviceInfo.data()->name();
	}

	return QString();
}
