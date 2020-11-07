// SPDX-License-Identifier: GPL-2.0
#include <QShortcut>
#include <QDebug>
#include <QMessageBox>
#include <QMenu>
#include "core/btdiscovery.h"

#include <QBluetoothUuid>

#include "ui_btdeviceselectiondialog.h"
#include "btdeviceselectiondialog.h"

BtDeviceSelectionDialog::BtDeviceSelectionDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::BtDeviceSelectionDialog),
	remoteDeviceDiscoveryAgent(0)
{
	ui->setupUi(this);

	// Quit button callbacks
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), this, SLOT(reject()));
	connect(ui->quit, SIGNAL(clicked()), this, SLOT(reject()));

	// Translate the UI labels
	ui->localDeviceDetails->setTitle(tr("Local Bluetooth device details"));
	ui->selectDeviceLabel->setText(tr("Select device:"));
	ui->deviceAddressLabel->setText(tr("Address:"));
	ui->deviceNameLabel->setText(tr("Name:"));
	ui->deviceState->setText(tr("Bluetooth powered on"));
	ui->changeDeviceState->setText(tr("Turn on/off"));
	ui->discoveredDevicesLabel->setText(tr("Discovered devices"));
	ui->scan->setText(tr("Scan"));
	ui->clear->setText(tr("Clear"));
	ui->save->setText(tr("Save"));
	ui->save->setDefault(true);
	ui->quit->setText(tr("Quit"));

	// Disable the save button because there is no device selected
	ui->save->setEnabled(false);

	// Add event for item selection
	connect(ui->discoveredDevicesList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
		this, SLOT(currentItemChanged(QListWidgetItem*,QListWidgetItem*)));

	// Initialize the local Bluetooth device
	localDevice = new QBluetoothLocalDevice();

	// Populate the list with local bluetooth devices
	QList<QBluetoothHostInfo> localAvailableDevices = localDevice->allDevices();
	int availableDevicesSize = localAvailableDevices.size();

	if (availableDevicesSize > 1) {
		int defaultDeviceIndex = -1;

		for (int it = 0; it < availableDevicesSize; it++) {
			QBluetoothHostInfo localAvailableDevice = localAvailableDevices.at(it);
			ui->localSelectedDevice->addItem(localAvailableDevice.name(),
							 QVariant::fromValue(localAvailableDevice.address()));

			if (localDevice->address() == localAvailableDevice.address())
				defaultDeviceIndex = it;
		}

		// Positionate the current index to the default device and register to index changes events
		ui->localSelectedDevice->setCurrentIndex(defaultDeviceIndex);
		connect(ui->localSelectedDevice, SIGNAL(currentIndexChanged(int)),
			this, SLOT(localDeviceChanged(int)));
	} else {
		// If there is only one local Bluetooth adapter hide the combobox and the label
		ui->selectDeviceLabel->hide();
		ui->localSelectedDevice->hide();
	}

	// Update the UI information about the local device
	updateLocalDeviceInformation();

	// Initialize the device discovery agent
	if (localDevice->isValid())
		initializeDeviceDiscoveryAgent();
}

BtDeviceSelectionDialog::~BtDeviceSelectionDialog()
{
	delete ui;

	// Clean the local device
	delete localDevice;

	if (remoteDeviceDiscoveryAgent) {
		// Clean the device discovery agent
		if (remoteDeviceDiscoveryAgent->isActive())
			remoteDeviceDiscoveryAgent->stop();
		delete remoteDeviceDiscoveryAgent;
	}
}

void BtDeviceSelectionDialog::on_changeDeviceState_clicked()
{
	if (localDevice->hostMode() == QBluetoothLocalDevice::HostPoweredOff) {
		ui->dialogStatus->setText(tr("Trying to turn on the local Bluetooth device..."));
		localDevice->powerOn();
	} else {
		ui->dialogStatus->setText(tr("Trying to turn off the local Bluetooth device..."));
		localDevice->setHostMode(QBluetoothLocalDevice::HostPoweredOff);
	}
}

void BtDeviceSelectionDialog::on_save_clicked()
{
	// Get the selected device. There will be always a selected device if the save button is enabled.
	QListWidgetItem *currentItem = ui->discoveredDevicesList->currentItem();
	QBluetoothDeviceInfo remoteDeviceInfo = currentItem->data(Qt::UserRole).value<QBluetoothDeviceInfo>();

	// Save the selected device
	selectedRemoteDeviceInfo.reset(new QBluetoothDeviceInfo(remoteDeviceInfo));
	QString address = remoteDeviceInfo.address().isNull() ? remoteDeviceInfo.deviceUuid().toString() :
								remoteDeviceInfo.address().toString();
	saveBtDeviceInfo(address, remoteDeviceInfo);
	if (remoteDeviceDiscoveryAgent->isActive()) {
		// Stop the SDP agent if the clear button is pressed and enable the Scan button
		remoteDeviceDiscoveryAgent->stop();
		ui->scan->setEnabled(true);
	}

	// Close the device selection dialog and set the result code to Accepted
	accept();
}

void BtDeviceSelectionDialog::on_clear_clicked()
{
	ui->dialogStatus->setText(tr("Remote devices list was cleared."));
	ui->discoveredDevicesList->clear();

	if (remoteDeviceDiscoveryAgent->isActive()) {
		// Stop the SDP agent if the clear button is pressed and enable the Scan button
		remoteDeviceDiscoveryAgent->stop();
		ui->scan->setEnabled(true);
	}
}

void BtDeviceSelectionDialog::on_scan_clicked()
{
	ui->dialogStatus->setText(tr("Scanning for remote devices..."));
	ui->discoveredDevicesList->clear();
	remoteDeviceDiscoveryAgent->start();
	ui->scan->setEnabled(false);
}

void BtDeviceSelectionDialog::remoteDeviceScanFinished()
{
	// This check is not necessary for Qt's QBluetoothDeviceDiscoveryAgent,
	// but with the home-brew WinBluetoothDeviceDiscoveryAgent, on error we
	// get an error() and an finished() signal. Thus, don't overwrite the
	// error message with a success message.
	if (remoteDeviceDiscoveryAgent->error() == QBluetoothDeviceDiscoveryAgent::NoError)
		ui->dialogStatus->setText(tr("Scanning finished successfully."));

	ui->scan->setEnabled(true);
}

void BtDeviceSelectionDialog::hostModeStateChanged(QBluetoothLocalDevice::HostMode mode)
{
	bool on = !(mode == QBluetoothLocalDevice::HostPoweredOff);

	//: %1 will be replaced with "turned on" or "turned off"
	ui->dialogStatus->setText(tr("The local Bluetooth device was %1.")
				  .arg(on? tr("turned on") : tr("turned off")));
	ui->deviceState->setChecked(on);
	ui->scan->setEnabled(on);
}

void BtDeviceSelectionDialog::addRemoteDevice(const QBluetoothDeviceInfo &remoteDeviceInfo)
{
	// are we supposed to show all devices or just dive computers?
	if (!ui->showNonDivecomputers->isChecked() && !matchesKnownDiveComputerNames(remoteDeviceInfo.name()))
		return;
#if defined(Q_OS_WIN)
	// On Windows we cannot obtain the pairing status so we set only the name and the address of the device
	QString deviceLabel = QString("%1 (%2)").arg(remoteDeviceInfo.name(),
						     remoteDeviceInfo.address().toString());
	QColor pairingColor = QColor(Qt::gray);
#else
	// By default we use the status label and the color for the UNPAIRED state
	QColor pairingColor = QColor("#F1A9A0");
	QString pairingStatusLabel = tr("UNPAIRED");
	QBluetoothLocalDevice::Pairing pairingStatus = localDevice->pairingStatus(remoteDeviceInfo.address());

	if (pairingStatus == QBluetoothLocalDevice::Paired) {
		pairingStatusLabel = tr("PAIRED");
		pairingColor = QColor(Qt::gray);
	} else if (pairingStatus == QBluetoothLocalDevice::AuthorizedPaired) {
		pairingStatusLabel = tr("AUTHORIZED_PAIRED");
		pairingColor = QColor("#89C4F4");
	}
	if (remoteDeviceInfo.address().isNull())
		pairingColor = QColor(Qt::gray);

	QString deviceLabel;

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
	if (!remoteDeviceInfo.deviceUuid().isNull()) {
		// we have only a Uuid, no address, so show that and reset the pairing color
		deviceLabel = QString("%1 (%2)").arg(remoteDeviceInfo.name(),remoteDeviceInfo.deviceUuid().toString());
		pairingColor =  QColor(Qt::gray);
	} else
#endif
	deviceLabel = tr("%1 (%2)   [State: %3]").arg(remoteDeviceInfo.name(),
							      remoteDeviceInfo.address().toString(),
							      pairingStatusLabel);
#endif
	// Create the new item, set its information and add it to the list
	QListWidgetItem *item = new QListWidgetItem(deviceLabel);

	item->setData(Qt::UserRole, QVariant::fromValue(remoteDeviceInfo));
	item->setBackground(pairingColor);

	ui->discoveredDevicesList->addItem(item);
}

void BtDeviceSelectionDialog::currentItemChanged(QListWidgetItem *item, QListWidgetItem *)
{
	// If the list is cleared, we get a signal with a null item pointer
	if (!item) {
		ui->save->setEnabled(false);
		return;
	}

	// By default we assume that the devices are paired
	QBluetoothDeviceInfo remoteDeviceInfo = item->data(Qt::UserRole).value<QBluetoothDeviceInfo>();
	QString statusMessage = tr("The device %1 can be used for connection. You can press the Save button.")
				  .arg(remoteDeviceInfo.address().isNull() ?
					       remoteDeviceInfo.deviceUuid().toString() :
					       remoteDeviceInfo.address().toString());
	bool enableSaveButton = true;

#if !defined(Q_OS_WIN)
	// On platforms other than Windows we can obtain the pairing status so if the devices are non-BLE devices
	// and not paired we disable the button
	// on MacOS some devices (including all BLE devices) only give us a Uuid and not and address; for those
	// we have no pairing status, either
	if (!remoteDeviceInfo.address().isNull() &&
	    remoteDeviceInfo.coreConfigurations() != QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
		QBluetoothLocalDevice::Pairing pairingStatus = localDevice->pairingStatus(remoteDeviceInfo.address());

		if (pairingStatus == QBluetoothLocalDevice::Unpaired) {
			statusMessage = tr("The device %1 must be paired in order to be used. Please use the context menu for pairing options.")
					.arg(remoteDeviceInfo.address().toString());
			enableSaveButton = false;
		}
	}
	if (remoteDeviceInfo.address().isNull() && remoteDeviceInfo.deviceUuid().isNull()) {
		statusMessage = tr("A device needs a non-zero address for a connection.");
		enableSaveButton = false;
	}
#endif
	// Update the status message and the save button
	ui->dialogStatus->setText(statusMessage);
	ui->save->setEnabled(enableSaveButton);
}

void BtDeviceSelectionDialog::localDeviceChanged(int index)
{
	QBluetoothAddress localDeviceSelectedAddress = ui->localSelectedDevice->itemData(index, Qt::UserRole).value<QBluetoothAddress>();

	// Delete the old localDevice
	if (localDevice)
		delete localDevice;

	// Create a new local device using the selected address
	localDevice = new QBluetoothLocalDevice(localDeviceSelectedAddress);

	ui->dialogStatus->setText(tr("The local device was changed."));

	// Clear the discovered devices list
	on_clear_clicked();

	// Update the UI information about the local device
	updateLocalDeviceInformation();

	// Initialize the device discovery agent
	if (localDevice->isValid())
		initializeDeviceDiscoveryAgent();
}

void BtDeviceSelectionDialog::displayPairingMenu(const QPoint &pos)
{
	QMenu menu(this);
	QAction *pairAction = menu.addAction(tr("Pair"));
	QAction *removePairAction = menu.addAction(tr("Remove pairing"));
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
		ui->dialogStatus->setText(tr("Trying to pair device %1")
					    .arg(currentRemoteDeviceInfo.address().toString()));
		localDevice->requestPairing(currentRemoteDeviceInfo.address(), QBluetoothLocalDevice::Paired);
	} else if (chosenAction == removePairAction) {
		ui->dialogStatus->setText(tr("Trying to unpair device %1")
					    .arg(currentRemoteDeviceInfo.address().toString()));
		localDevice->requestPairing(currentRemoteDeviceInfo.address(), QBluetoothLocalDevice::Unpaired);
	}
}

void BtDeviceSelectionDialog::pairingFinished(const QBluetoothAddress &address, QBluetoothLocalDevice::Pairing pairing)
{
	// Determine the color, the new pairing status and the log message. By default we assume that the devices are UNPAIRED.
	QString remoteDeviceStringAddress = address.toString();
	QColor pairingColor = QColor(Qt::red);
	QString pairingStatusLabel = tr("UNPAIRED");
	QString dialogStatusMessage = tr("Device %1 was unpaired.").arg(remoteDeviceStringAddress);
	bool enableSaveButton = false;

	if (pairing == QBluetoothLocalDevice::Paired) {
		pairingStatusLabel = tr("PAIRED");
		pairingColor = QColor(Qt::gray);
		enableSaveButton = true;
		dialogStatusMessage = tr("Device %1 was paired.").arg(remoteDeviceStringAddress);
	} else if (pairing == QBluetoothLocalDevice::AuthorizedPaired) {
		pairingStatusLabel = tr("AUTHORIZED_PAIRED");
		pairingColor = QColor(Qt::blue);
		enableSaveButton = true;
		dialogStatusMessage = tr("Device %1 was paired and is authorized.").arg(remoteDeviceStringAddress);
	}

	// Find the items which represent the BTH device and update their state
	QList<QListWidgetItem *> items = ui->discoveredDevicesList->findItems(remoteDeviceStringAddress, Qt::MatchContains);
	QRegularExpression pairingExpression(QString("%1|%2|%3").arg(tr("PAIRED"),
								     tr("AUTHORIZED_PAIRED"),
								     tr("UNPAIRED")));

	for (int i = 0; i < items.count(); ++i) {
		QListWidgetItem *item = items.at(i);
		QString updatedDeviceLabel = item->text().replace(pairingExpression, pairingStatusLabel);

		item->setText(updatedDeviceLabel);
		item->setBackground(pairingColor);
	}

	// Check if the updated device is the selected one from the list and inform the user that it can/cannot start the download mode
	QListWidgetItem *currentItem = ui->discoveredDevicesList->currentItem();

	if (currentItem != NULL && currentItem->text().contains(remoteDeviceStringAddress, Qt::CaseInsensitive)) {
		if (pairing == QBluetoothLocalDevice::Unpaired) {
			dialogStatusMessage = tr("The device %1 must be paired in order to be used. Please use the context menu for pairing options.")
						.arg(remoteDeviceStringAddress);
		} else {
			dialogStatusMessage = tr("The device %1 can now be used for connection. You can press the Save button.")
						.arg(remoteDeviceStringAddress);
		}
	}

	// Update the save button and the dialog status message
	ui->save->setEnabled(enableSaveButton);
	ui->dialogStatus->setText(dialogStatusMessage);
}

void BtDeviceSelectionDialog::error(QBluetoothLocalDevice::Error error)
{
	ui->dialogStatus->setText(tr("Local device error: %1.")
				    .arg((error == QBluetoothLocalDevice::PairingError)? tr("Pairing error. If the remote device requires a custom PIN code, "
											    "please try to pair the devices using your operating system. ")
										       : tr("Unknown error")));
}

void BtDeviceSelectionDialog::deviceDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error error)
{
	QString errorDescription;

	switch (error) {
	case QBluetoothDeviceDiscoveryAgent::PoweredOffError:
		errorDescription = tr("The Bluetooth adaptor is powered off, power it on before doing discovery.");
		break;
	case QBluetoothDeviceDiscoveryAgent::InputOutputError:
		errorDescription = tr("Writing to or reading from the device resulted in an error.");
		break;
	default:
		errorDescription = tr("An unknown error has occurred.");
		break;
	}

	ui->dialogStatus->setText(tr("Device discovery error: %1.").arg(errorDescription));
}

extern QString markBLEAddress(const QBluetoothDeviceInfo *device);
extern QString btDeviceAddress(const QBluetoothDeviceInfo *device, bool isBle);

QString BtDeviceSelectionDialog::getSelectedDeviceAddress()
{
	if (!selectedRemoteDeviceInfo)
		return QString();

	int btMode = ui->btMode->currentIndex();
	QBluetoothDeviceInfo *device = selectedRemoteDeviceInfo.data();

	switch (btMode) {
	case 0:		// Auto
	default:
		return markBLEAddress(device);
	case 1:		// Force LE
		return btDeviceAddress(device, true);
	case 2:		// Force classical
		return btDeviceAddress(device, false);
	}
}

QString BtDeviceSelectionDialog::getSelectedDeviceName()
{
	if (selectedRemoteDeviceInfo)
		return selectedRemoteDeviceInfo.data()->name();

	return QString();
}

QString BtDeviceSelectionDialog::getSelectedDeviceText()
{
	return formatDeviceText(getSelectedDeviceAddress(), getSelectedDeviceName());
}

QString BtDeviceSelectionDialog::formatDeviceText(const QString &address, const QString &name)
{
	if (address.isEmpty())
		return name;
	if (name.isEmpty())
		return address;
	return QString("%1 (%2)").arg(name, address);
}

void BtDeviceSelectionDialog::updateLocalDeviceInformation()
{
	// Check if the selected Bluetooth device can be accessed
	if (!localDevice->isValid()) {
		QString na = tr("Not available");

		// Update the UI information
		ui->deviceAddress->setText(na);
		ui->deviceName->setText(na);

		// Announce the user that there is a problem with the selected local Bluetooth adapter
		ui->dialogStatus->setText(tr("The local Bluetooth adapter cannot be accessed."));

		// Disable the buttons
		ui->save->setEnabled(false);
		ui->scan->setEnabled(false);
		ui->clear->setEnabled(false);
		ui->changeDeviceState->setEnabled(false);

		return;
	}

	// Set UI information about the local device
	ui->deviceAddress->setText(localDevice->address().toString());
	ui->deviceName->setText(localDevice->name());

	connect(localDevice, SIGNAL(hostModeStateChanged(QBluetoothLocalDevice::HostMode)),
		this, SLOT(hostModeStateChanged(QBluetoothLocalDevice::HostMode)));

	// Initialize the state of the local device and activate/deactive the scan button
	hostModeStateChanged(localDevice->hostMode());

	// Add context menu for devices to be able to pair them
	ui->discoveredDevicesList->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->discoveredDevicesList, SIGNAL(customContextMenuRequested(QPoint)),
		this, SLOT(displayPairingMenu(QPoint)));
	connect(localDevice, SIGNAL(pairingFinished(QBluetoothAddress, QBluetoothLocalDevice::Pairing)),
		this, SLOT(pairingFinished(QBluetoothAddress, QBluetoothLocalDevice::Pairing)));

	connect(localDevice, SIGNAL(error(QBluetoothLocalDevice::Error)),
		this, SLOT(error(QBluetoothLocalDevice::Error)));
}

void BtDeviceSelectionDialog::initializeDeviceDiscoveryAgent()
{
	// Intialize the discovery agent
	remoteDeviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(localDevice->address());

	// Test if the discovery agent was successfully created
	if (remoteDeviceDiscoveryAgent->error() == QBluetoothDeviceDiscoveryAgent::InvalidBluetoothAdapterError) {
		ui->dialogStatus->setText(tr("The device discovery agent was not created because the %1 address does not "
					     "match the physical adapter address of any local Bluetooth device.")
					    .arg(localDevice->address().toString()));
		ui->scan->setEnabled(false);
		ui->clear->setEnabled(false);
		return;
	}
	connect(remoteDeviceDiscoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)),
		this, SLOT(addRemoteDevice(QBluetoothDeviceInfo)));
	connect(remoteDeviceDiscoveryAgent, SIGNAL(finished()),
		this, SLOT(remoteDeviceScanFinished()));
	connect(remoteDeviceDiscoveryAgent, SIGNAL(error(QBluetoothDeviceDiscoveryAgent::Error)),
		this, SLOT(deviceDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error)));
}
