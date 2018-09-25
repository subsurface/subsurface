// SPDX-License-Identifier: GPL-2.0
#include <QShortcut>
#include <QDebug>
#include <QMessageBox>
#include <QMenu>
#include <QShowEvent>
#include "core/btdiscovery.h"

#include <QBluetoothUuid>
#include <QSettings>

#include "ui_btdeviceselectiondialog.h"
#include "btdeviceselectiondialog.h"

#if defined(Q_OS_WIN)
Q_DECLARE_METATYPE(QBluetoothDeviceDiscoveryAgent::Error)
#endif
#if QT_VERSION < 0x050500
Q_DECLARE_METATYPE(QBluetoothDeviceInfo)
#endif

BtDeviceSelectionDialog::BtDeviceSelectionDialog(const QString &address, dc_descriptor_t *dc, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::BtDeviceSelectionDialog),
	previousDevice(address),
	dcDescriptor(dc),
	maxPriority(0),
	remoteDeviceDiscoveryAgent(0)
{
	ui->setupUi(this);

	// Quit button callbacks
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), this, SLOT(on_quit_clicked()));

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
	setScanStatusInvalid();

	ui->waitingSpinner->setRoundness(70.0);
	ui->waitingSpinner->setMinimumTrailOpacity(15.0);
	ui->waitingSpinner->setTrailFadePercentage(70.0);
	ui->waitingSpinner->setNumberOfLines(8);
	ui->waitingSpinner->setLineLength(5);
	ui->waitingSpinner->setLineWidth(3);
	ui->waitingSpinner->setInnerRadius(5);
	ui->waitingSpinner->setRevolutionsPerSecond(1);

	// Disable the save button because there is no device selected
	ui->save->setEnabled(false);

	// Add event for item selection
	connect(ui->discoveredDevicesList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
		this, SLOT(currentItemChanged(QListWidgetItem*,QListWidgetItem*)));

	// Remove BLE marker from address
	if (previousDevice.startsWith("LE:"))
		previousDevice = previousDevice.mid(3);

#if defined(Q_OS_WIN)
	ULONG       ulRetCode = SUCCESS;
	WSADATA     WSAData = { 0 };

	// Initialize WinSock and ask for version 2.2.
	ulRetCode = WSAStartup(MAKEWORD(2, 2), &WSAData);
	if (ulRetCode != SUCCESS) {
		QMessageBox::critical(this, "Bluetooth",
						   tr("Could not initialize Winsock version 2.2"), QMessageBox::Ok);
		return;
	}

	// Initialize the device discovery agent
	initializeDeviceDiscoveryAgent();

	// On Windows we cannot select a device or show information about the local device
	ui->localDeviceDetails->hide();
#else
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
#endif
	loadDeviceList();
}

BtDeviceSelectionDialog::~BtDeviceSelectionDialog()
{
	delete ui;

	if (remoteDeviceDiscoveryAgent)
		delete remoteDeviceDiscoveryAgent;
#if defined(Q_OS_WIN)
	// Terminate the use of Winsock 2 DLL
	WSACleanup();
#else
	// Clean the local device
	delete localDevice;
#endif
}

void BtDeviceSelectionDialog::on_changeDeviceState_clicked()
{
#if defined(Q_OS_WIN)
	// TODO add implementation
#else
	if (isPoweredOn()) {
		ui->dialogStatus->setText(tr("Trying to turn off the local Bluetooth device..."));
		localDevice->setHostMode(QBluetoothLocalDevice::HostPoweredOff);
	} else {
		ui->dialogStatus->setText(tr("Trying to turn on the local Bluetooth device..."));
		localDevice->powerOn();
	}
#endif
}

static quint32 deviceInfoToClass(const QBluetoothDeviceInfo &info)
{
	return (info.minorDeviceClass() << 2) |
	       (info.majorDeviceClass() << 8) |
	       ((quint32)info.serviceClasses() << 13);
}

static QString addressOrUuid(const QBluetoothDeviceInfo &info)
{
	return info.address().isNull() ? info.deviceUuid().toString() : info.address().toString();
}

// Compare Bluetooth devices by address or UUID.
// When using QBluetoothDeviceInfo::operator==(), scanned and loaded (from the settings)
// device-infos don't compare as equal, for yet unknown reasons.
static bool sameDevice(const QBluetoothDeviceInfo &d1, const QBluetoothDeviceInfo &d2)
{
	return addressOrUuid(d1) == addressOrUuid(d2);
}

void BtDeviceSelectionDialog::saveDeviceList()
{
	int numRows = ui->discoveredDevicesList->count();
	QSettings s;
	s.beginGroup("Bluetooth");
	s.beginWriteArray("devices", numRows);
	for (int i = 0; i < numRows; ++i) {
		QBluetoothDeviceInfo info = ui->discoveredDevicesList->item(i)->data(Qt::UserRole).value<QBluetoothDeviceInfo>();
		s.setArrayIndex(i);
		s.setValue("name", info.name());
		s.setValue("address", addressOrUuid(info));
		s.setValue("class", deviceInfoToClass(info));
	}
	s.endArray();
	s.endGroup();
}

void BtDeviceSelectionDialog::loadDeviceList()
{
	QSettings s;
	s.beginGroup("Bluetooth");
	int numRows = s.beginReadArray("devices");
	for (int i = 0; i < numRows; ++i) {
		s.setArrayIndex(i);
		QBluetoothDeviceInfo info(
			// For Mac, we saved an UUID, for Windows and Linux the Bluetooth address.
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
			QBluetoothUuid(s.value("address").toString()),
#else
			QBluetoothAddress(s.value("address").toString()),
#endif
			s.value("name").toString(),
			s.value("class").toUInt()
		);
		addRemoteDevice(info);
	}
	s.endArray();
	s.endGroup();
}

void BtDeviceSelectionDialog::on_save_clicked()
{
	// Get the selected device. There will be always a selected device if the save button is enabled.
	QListWidgetItem *currentItem = ui->discoveredDevicesList->currentItem();
	QBluetoothDeviceInfo remoteDeviceInfo = currentItem->data(Qt::UserRole).value<QBluetoothDeviceInfo>();

	// Save the selected device
	selectedRemoteDeviceInfo.reset(new QBluetoothDeviceInfo(remoteDeviceInfo));
	QString address = addressOrUuid(remoteDeviceInfo);
	saveBtDeviceInfo(address, remoteDeviceInfo);
	previousDevice = address;
	stopScan();
	saveDeviceList();

	// Close the device selection dialog and set the result code to Accepted
	accept();
}

void BtDeviceSelectionDialog::on_quit_clicked()
{
	stopScan();
	saveDeviceList();

	// Close the device selection dialog and set the result code to Rejected
	reject();
}

void BtDeviceSelectionDialog::showEvent(QShowEvent *event)
{
	// Remove any status message from a previous display
	ui->dialogStatus->clear();
}

void BtDeviceSelectionDialog::on_clear_clicked()
{
	if (remoteDeviceDiscoveryAgent->isActive())
		stopScan();

	ui->dialogStatus->setText(tr("Remote devices list was cleared."));
	ui->discoveredDevicesList->clear();
	maxPriority = 0;
}

void BtDeviceSelectionDialog::on_scan_clicked()
{
	if (remoteDeviceDiscoveryAgent->isActive())
		stopScan();
	else
		startScan();
}

void BtDeviceSelectionDialog::remoteDeviceScanFinished()
{
	// This check is not necessary for Qt's QBluetoothDeviceDiscoveryAgent,
	// but with the home-brew WinBluetoothDeviceDiscoveryAgent, on error we
	// get an error() and an finished() signal. Thus, don't overwrite the
	// error message with a success message.
	if (remoteDeviceDiscoveryAgent->error() == QBluetoothDeviceDiscoveryAgent::NoError)
		ui->dialogStatus->setText(tr("Scanning finished successfully."));

	setScanStatusOff();
}

bool BtDeviceSelectionDialog::isPoweredOn() const
{
#if defined(Q_OS_WIN)
	// TODO add implementation
	return true;
#else
	return localDevice->hostMode() != QBluetoothLocalDevice::HostPoweredOff;
#endif
}

void BtDeviceSelectionDialog::setScanStatusOn()
{
	ui->scanStatus->setText(tr("Scanning..."));
	ui->scan->setText(tr("Stop scan"));
	ui->scan->setEnabled(true);
	ui->clear->setEnabled(true);
	ui->waitingSpinner->start();
}

void BtDeviceSelectionDialog::setScanStatusOff()
{
	ui->scanStatus->setText(tr("Idle"));
	ui->scan->setText(tr("Start scan"));
	ui->scan->setEnabled(true);
	ui->clear->setEnabled(true);
	ui->waitingSpinner->stop();
}

void BtDeviceSelectionDialog::setScanStatusInvalid()
{
	ui->scanStatus->setText("-");
	ui->scan->setText(tr("Start scan"));
	ui->scan->setEnabled(false);
	ui->clear->setEnabled(false);
	ui->waitingSpinner->stop();
}

void BtDeviceSelectionDialog::stopScan()
{
	if (remoteDeviceDiscoveryAgent && remoteDeviceDiscoveryAgent->isActive()) {
		remoteDeviceDiscoveryAgent->stop();
		ui->dialogStatus->setText(tr("Stopped scanning..."));
	}
	if (isPoweredOn())
		setScanStatusOff();
	else
		setScanStatusInvalid();
}

void BtDeviceSelectionDialog::startScan()
{
	if (remoteDeviceDiscoveryAgent) {
		maxPriority = 0;
		remoteDeviceDiscoveryAgent->start();
		ui->dialogStatus->setText(tr("Scanning for remote devices..."));
		setScanStatusOn();
	}
}

void BtDeviceSelectionDialog::hostModeStateChanged(QBluetoothLocalDevice::HostMode mode)
{
#if defined(Q_OS_WIN)
	// TODO add implementation
#else
	bool on = !(mode == QBluetoothLocalDevice::HostPoweredOff);

	//: %1 will be replaced with "turned on" or "turned off"
	ui->dialogStatus->setText(tr("The local Bluetooth device was %1.")
				  .arg(on? tr("turned on") : tr("turned off")));
	ui->deviceState->setChecked(on);
#endif
}

int BtDeviceSelectionDialog::getDevicePriority(bool connectable, const QBluetoothDeviceInfo &remoteDeviceInfo)
{
	if (!connectable)
		return 0;
	QString address = remoteDeviceInfo.address().isNull() ?
		remoteDeviceInfo.deviceUuid().toString() :
		remoteDeviceInfo.address().toString();
	if (address == previousDevice)
		return 4;
	if (dc_descriptor_t *dc = getDeviceType(remoteDeviceInfo.name()))
		return dc == dcDescriptor ? 3 : 2;
	return 1;
}

void BtDeviceSelectionDialog::addRemoteDevice(const QBluetoothDeviceInfo &remoteDeviceInfo)
{
	if (!remoteDeviceInfo.isValid())
		return;
#if defined(Q_OS_WIN)
	// On Windows we cannot obtain the pairing status so we set only the name and the address of the device
	bool connectable = true;
	QString deviceLabel = QString("%1 (%2)").arg(remoteDeviceInfo.name(),
						     remoteDeviceInfo.address().toString());
	QColor pairingColor = QColor(Qt::white);
#else
	// By default we use the status label and the color for the UNPAIRED state
	QColor pairingColor = QColor("#F1A9A0");
	QString pairingStatusLabel = tr("UNPAIRED");
	QBluetoothLocalDevice::Pairing pairingStatus = localDevice->pairingStatus(remoteDeviceInfo.address());

	bool connectable = false;
	if (pairingStatus == QBluetoothLocalDevice::Paired) {
		pairingStatusLabel = tr("PAIRED");
		pairingColor = QColor(Qt::gray);
		connectable = true;
	} else if (pairingStatus == QBluetoothLocalDevice::AuthorizedPaired) {
		pairingStatusLabel = tr("AUTHORIZED_PAIRED");
		pairingColor = QColor("#89C4F4");
		connectable = true;
	}
	if (remoteDeviceInfo.address().isNull())
		pairingColor = QColor(Qt::gray);

	QString deviceLabel;

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
	if (!remoteDeviceInfo.deviceUuid().isNull()) {
		// we have only a Uuid, no address, so show that and reset the pairing color
		deviceLabel = QString("%1 (%2)").arg(remoteDeviceInfo.name(),remoteDeviceInfo.deviceUuid().toString());
		pairingColor = QColor(Qt::white);
		connectable = true;
	} else
#endif
	deviceLabel = tr("%1 (%2)   [State: %3]").arg(remoteDeviceInfo.name(),
							      remoteDeviceInfo.address().toString(),
							      pairingStatusLabel);
#endif
	// Check if item is already in list. Add it, if it isn't
	int numRows = ui->discoveredDevicesList->count();
	int row;
	QListWidgetItem *item;
	for (row = 0; row < numRows; ++row) {
		item = ui->discoveredDevicesList->item(row);
		if (sameDevice(item->data(Qt::UserRole).value<QBluetoothDeviceInfo>(), remoteDeviceInfo))
			break;
	}

	if (row >= numRows) {
		// Create the new item and add it to the list
		item = new QListWidgetItem;
		ui->discoveredDevicesList->addItem(item);
	}

	item->setText(deviceLabel);
	item->setData(Qt::UserRole, QVariant::fromValue(remoteDeviceInfo));
	item->setBackgroundColor(pairingColor);

	int priority = getDevicePriority(connectable, remoteDeviceInfo);
	if (priority > maxPriority) {
		maxPriority = priority;
		ui->discoveredDevicesList->setCurrentItem(item);
		ui->save->setEnabled(true);
		ui->save->setFocus();
	}
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
	// On other platforms than Windows we can obtain the pairing status so if the devices are not paired we disable the button
	// except on MacOS for those devices that only give us a Uuid and not and address (as we have no pairing status for those, either)
	if (!remoteDeviceInfo.address().isNull()) {
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
#if defined(Q_OS_WIN)
	// TODO add implementation
#else
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
#endif
}

void BtDeviceSelectionDialog::displayPairingMenu(const QPoint &pos)
{
#if defined(Q_OS_WIN)
	// TODO add implementation
#else
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
#endif
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
	QRegularExpression pairingExpression = QRegularExpression(QString("%1|%2|%3").arg(tr("PAIRED"),
											  tr("AUTHORIZED_PAIRED"),
											  tr("UNPAIRED")));

	for (int i = 0; i < items.count(); ++i) {
		QListWidgetItem *item = items.at(i);
		QString updatedDeviceLabel = item->text().replace(QRegularExpression(pairingExpression),
								  pairingStatusLabel);

		item->setText(updatedDeviceLabel);
		item->setBackgroundColor(pairingColor);
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
#if defined(Q_OS_WIN)
		errorDescription = remoteDeviceDiscoveryAgent->errorToString();
#else
		errorDescription = tr("An unknown error has occurred.");
#endif
		break;
	}

	ui->dialogStatus->setText(tr("Device discovery error: %1.").arg(errorDescription));
	setScanStatusOff();
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
#if defined(Q_OS_WIN)
	// TODO add implementation
#else
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
		ui->changeDeviceState->setEnabled(false);
		setScanStatusInvalid();

		return;
	}

	// Set UI information about the local device
	ui->deviceAddress->setText(localDevice->address().toString());
	ui->deviceName->setText(localDevice->name());

	connect(localDevice, SIGNAL(hostModeStateChanged(QBluetoothLocalDevice::HostMode)),
		this, SLOT(hostModeStateChanged(QBluetoothLocalDevice::HostMode)));

	// Initialize the state of the local device
	hostModeStateChanged(localDevice->hostMode());

	// Add context menu for devices to be able to pair them
	ui->discoveredDevicesList->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->discoveredDevicesList, SIGNAL(customContextMenuRequested(QPoint)),
		this, SLOT(displayPairingMenu(QPoint)));
	connect(localDevice, SIGNAL(pairingFinished(QBluetoothAddress, QBluetoothLocalDevice::Pairing)),
		this, SLOT(pairingFinished(QBluetoothAddress, QBluetoothLocalDevice::Pairing)));

	connect(localDevice, SIGNAL(error(QBluetoothLocalDevice::Error)),
		this, SLOT(error(QBluetoothLocalDevice::Error)));
#endif
}

void BtDeviceSelectionDialog::initializeDeviceDiscoveryAgent()
{
#if defined(Q_OS_WIN)
	// Register QBluetoothDeviceInfo metatype
	qRegisterMetaType<QBluetoothDeviceInfo>();

	// Register QBluetoothDeviceDiscoveryAgent metatype (Needed for QBluetoothDeviceDiscoveryAgent::Error)
	qRegisterMetaType<QBluetoothDeviceDiscoveryAgent::Error>();

	// Intialize the discovery agent
	remoteDeviceDiscoveryAgent = new WinBluetoothDeviceDiscoveryAgent(this);
#else
	// Intialize the discovery agent
	remoteDeviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(localDevice->address());

	// Test if the discovery agent was successfully created
	if (remoteDeviceDiscoveryAgent->error() == QBluetoothDeviceDiscoveryAgent::InvalidBluetoothAdapterError) {
		ui->dialogStatus->setText(tr("The device discovery agent was not created because the %1 address does not "
					     "match the physical adapter address of any local Bluetooth device.")
					    .arg(localDevice->address().toString()));
		setScanStatusInvalid();
		return;
	}
#endif
	connect(remoteDeviceDiscoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)),
		this, SLOT(addRemoteDevice(QBluetoothDeviceInfo)));
	connect(remoteDeviceDiscoveryAgent, SIGNAL(finished()),
		this, SLOT(remoteDeviceScanFinished()));
	connect(remoteDeviceDiscoveryAgent, SIGNAL(error(QBluetoothDeviceDiscoveryAgent::Error)),
		this, SLOT(deviceDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error)));
	if (isPoweredOn())
		setScanStatusOff();
	else
		setScanStatusInvalid();
}

#if defined(Q_OS_WIN)
WinBluetoothDeviceDiscoveryAgent::WinBluetoothDeviceDiscoveryAgent(QObject *parent) : QThread(parent),
	stopped(true),
	quit(false)
{
}

WinBluetoothDeviceDiscoveryAgent::~WinBluetoothDeviceDiscoveryAgent()
{
	{
		QMutexLocker l(&lock);
		quit = false;
	}
	cond.wakeOne();
	wait();
}

bool WinBluetoothDeviceDiscoveryAgent::isActive() const
{
	QMutexLocker l(&lock);
	return !stopped && !quit;
}

QString WinBluetoothDeviceDiscoveryAgent::errorToString() const
{
	QMutexLocker l(&lock);
	return lastErrorToString;
}

QBluetoothDeviceDiscoveryAgent::Error WinBluetoothDeviceDiscoveryAgent::error() const
{
	QMutexLocker l(&lock);
	return lastError;
}

void WinBluetoothDeviceDiscoveryAgent::reportError(QBluetoothDeviceDiscoveryAgent::Error errorCode)
{
	QMutexLocker l(&lock);
	// If we were stopped, don't bother reporting an error
	if (stopped || quit)
		return;
	lastErrorToString = qt_error_string();
	lastError = errorCode;
	stopped = true;
	l.unlock();
	emit error(errorCode);
}

void WinBluetoothDeviceDiscoveryAgent::doWork()
{
	WSAQUERYSET queryset;
	HANDLE hLookup;
	BYTE buffer[4096];
	WSAQUERYSET *pResults = (WSAQUERYSET*)&buffer;
	DWORD bufferLength = sizeof(buffer);

	lastError = QBluetoothDeviceDiscoveryAgent::NoError;
	lastErrorToString = tr("No error");

	// Initialize query for device and start the lookup service
	memset(&queryset, 0, sizeof(WSAQUERYSET));
	queryset.dwSize = sizeof(WSAQUERYSET);
	queryset.dwNameSpace = NS_BTH;

	// The LUP_CONTAINERS flag is used to signal that we are doing a device inquiry
	// while LUP_FLUSHCACHE flag is used to flush the device cache for all inquiries
	// and to do a fresh lookup instead.
	int result = WSALookupServiceBegin(&queryset, LUP_CONTAINERS | LUP_FLUSHCACHE, &hLookup);
	if (result != SUCCESS) {
		result = WSAGetLastError();
		qWarning() << "Couldn't start Bluetooth lookup service:" << result;
		// Get the last error and emit a signal
		reportError(QBluetoothDeviceDiscoveryAgent::PoweredOffError);
		return;
	}

	// Declare the necessary variables to collect the information
	memset(buffer, 0, sizeof(buffer));

	pResults->dwSize = sizeof(WSAQUERYSET);
	pResults->dwNameSpace = NS_BTH;
	pResults->lpBlob = NULL;

	while (isActive()) {
		// LUP_RETURN_NAME and LUP_RETURN_ADDR flags are used to return the name and the address of the discovered device
		int result = WSALookupServiceNext(hLookup, LUP_RETURN_NAME | LUP_RETURN_ADDR, &bufferLength, pResults);
		if (result != SUCCESS)
			result = WSAGetLastError();
		if (result == WSA_E_NO_MORE || result == WSAENOMORE)
			// No more entries -> start over
			break;

		if (result != SUCCESS) {
			// Get the last error and emit a signal
			qWarning() << "Unexpected error while fetching Bluetooth device:" << result;
			reportError(QBluetoothDeviceDiscoveryAgent::UnknownError);
			break;
		}

		// Found a device
		QString deviceAddress(BTH_ADDR_BUF_LEN, Qt::Uninitialized);
		DWORD addressSize = BTH_ADDR_BUF_LEN;

		// Collect the address of the device from the WSAQUERYSET
		SOCKADDR_BTH *socketBthAddress = (SOCKADDR_BTH *) pResults->lpcsaBuffer->RemoteAddr.lpSockaddr;

		// Convert the BTH_ADDR to string
		if ((result = WSAAddressToStringW((LPSOCKADDR) socketBthAddress,
					sizeof (*socketBthAddress),
					NULL,
					reinterpret_cast<wchar_t*>(deviceAddress.data()),
					&addressSize
					)) != SUCCESS) {
			// Get the last error and emit a signal
			result = WSAGetLastError();
			qWarning() << "Error translating address to string:" << result;
			reportError(QBluetoothDeviceDiscoveryAgent::UnknownError);
			break;
		}

		// If we were stopped, don't bother reporting the result
		if (!isActive())
			break;

		// Remove the round parentheses
		deviceAddress.remove(')');
		deviceAddress.remove('(');

		// Save the name of the discovered device and truncate the address
		QString deviceName = QString(pResults->lpszServiceInstanceName);
		deviceAddress.truncate(BTH_ADDR_PRETTY_STRING_LEN);

		// Create an object with information about the discovered device
		QBluetoothDeviceInfo deviceInfo(QBluetoothAddress(deviceAddress), deviceName, 0);

		// Raise a signal with information about the found remote device
		emit deviceDiscovered(deviceInfo);
	}
	if (result != SUCCESS) {
		result = WSAGetLastError();
		qWarning() << "Couldn't end Bluetooth lookup service:" << result;
	}
}

void WinBluetoothDeviceDiscoveryAgent::run()
{
	for (;;) {
		// Wait for either quit-signal or work-to-do
		for (;;) {
			QMutexLocker l(&lock);
			if (quit)
				return;
			if (!stopped)
				break;
			cond.wait(&lock);
		}
		doWork();
	}
}

void WinBluetoothDeviceDiscoveryAgent::stop()
{
	QMutexLocker l(&lock);
	stopped = true;
}

void WinBluetoothDeviceDiscoveryAgent::start()
{
	if (!isRunning())
		QThread::start();
	{
		QMutexLocker l(&lock);
		stopped = false;
	}
	cond.wakeOne();
}

#endif
