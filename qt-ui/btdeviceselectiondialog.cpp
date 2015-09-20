#include <QShortcut>
#include <QDebug>
#include <QMessageBox>
#include <QMenu>

#include "ui_btdeviceselectiondialog.h"
#include "btdeviceselectiondialog.h"

#if defined(Q_OS_WIN)
Q_DECLARE_METATYPE(QBluetoothDeviceDiscoveryAgent::Error)
#endif
#if QT_VERSION < 0x050500
Q_DECLARE_METATYPE(QBluetoothDeviceInfo)
#endif

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
	ui->quit->setText(tr("Quit"));

	// Disable the save button because there is no device selected
	ui->save->setEnabled(false);

	// Add event for item selection
	connect(ui->discoveredDevicesList, SIGNAL(itemClicked(QListWidgetItem*)),
		this, SLOT(itemClicked(QListWidgetItem*)));

#if defined(Q_OS_WIN)
	ULONG       ulRetCode = SUCCESS;
	WSADATA     WSAData = { 0 };

	// Initialize WinSock and ask for version 2.2.
	ulRetCode = WSAStartup(MAKEWORD(2, 2), &WSAData);
	if (ulRetCode != SUCCESS) {
		QMessageBox::StandardButton warningBox;
		warningBox = QMessageBox::critical(this, "Bluetooth",
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
}

BtDeviceSelectionDialog::~BtDeviceSelectionDialog()
{
	delete ui;

#if defined(Q_OS_WIN)
	// Terminate the use of Winsock 2 DLL
	WSACleanup();
#else
	// Clean the local device
	delete localDevice;
#endif
	if (remoteDeviceDiscoveryAgent) {
		// Clean the device discovery agent
		if (remoteDeviceDiscoveryAgent->isActive()) {
			remoteDeviceDiscoveryAgent->stop();
#if defined(Q_OS_WIN)
			remoteDeviceDiscoveryAgent->wait();
#endif
		}

		delete remoteDeviceDiscoveryAgent;
	}
}

void BtDeviceSelectionDialog::on_changeDeviceState_clicked()
{
#if defined(Q_OS_WIN)
	// TODO add implementation
#else
	if (localDevice->hostMode() == QBluetoothLocalDevice::HostPoweredOff) {
		ui->dialogStatus->setText(tr("Trying to turn on the local Bluetooth device..."));
		localDevice->powerOn();
	} else {
		ui->dialogStatus->setText(tr("Trying to turn off the local Bluetooth device..."));
		localDevice->setHostMode(QBluetoothLocalDevice::HostPoweredOff);
	}
#endif
}

void BtDeviceSelectionDialog::on_save_clicked()
{
	// Get the selected device. There will be always a selected device if the save button is enabled.
	QListWidgetItem *currentItem = ui->discoveredDevicesList->currentItem();
	QBluetoothDeviceInfo remoteDeviceInfo = currentItem->data(Qt::UserRole).value<QBluetoothDeviceInfo>();

	// Save the selected device
	selectedRemoteDeviceInfo = QSharedPointer<QBluetoothDeviceInfo>(new QBluetoothDeviceInfo(remoteDeviceInfo));

	if (remoteDeviceDiscoveryAgent->isActive()) {
		// Stop the SDP agent if the clear button is pressed and enable the Scan button
		remoteDeviceDiscoveryAgent->stop();
#if defined(Q_OS_WIN)
		remoteDeviceDiscoveryAgent->wait();
#endif
		ui->scan->setEnabled(true);
	}

	// Close the device selection dialog and set the result code to Accepted
	accept();
}

void BtDeviceSelectionDialog::on_clear_clicked()
{
	ui->dialogStatus->setText(tr("Remote devices list was cleared."));
	ui->discoveredDevicesList->clear();
	ui->save->setEnabled(false);

	if (remoteDeviceDiscoveryAgent->isActive()) {
		// Stop the SDP agent if the clear button is pressed and enable the Scan button
		remoteDeviceDiscoveryAgent->stop();
#if defined(Q_OS_WIN)
		remoteDeviceDiscoveryAgent->wait();
#endif
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
	if (remoteDeviceDiscoveryAgent->error() == QBluetoothDeviceDiscoveryAgent::NoError) {
		ui->dialogStatus->setText(tr("Scanning finished successfully."));
	} else {
		deviceDiscoveryError(remoteDeviceDiscoveryAgent->error());
	}

	ui->scan->setEnabled(true);
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
	ui->scan->setEnabled(on);
#endif
}

void BtDeviceSelectionDialog::addRemoteDevice(const QBluetoothDeviceInfo &remoteDeviceInfo)
{
#if defined(Q_OS_WIN)
	// On Windows we cannot obtain the pairing status so we set only the name and the address of the device
	QString deviceLabel = QString("%1 (%2)").arg(remoteDeviceInfo.name(),
						     remoteDeviceInfo.address().toString());
	QColor pairingColor = QColor(Qt::white);
#else
	// By default we use the status label and the color for the UNPAIRED state
	QColor pairingColor = QColor(Qt::red);
	QString pairingStatusLabel = tr("UNPAIRED");
	QBluetoothLocalDevice::Pairing pairingStatus = localDevice->pairingStatus(remoteDeviceInfo.address());

	if (pairingStatus == QBluetoothLocalDevice::Paired) {
		pairingStatusLabel = tr("PAIRED");
		pairingColor = QColor(Qt::gray);
	} else if (pairingStatus == QBluetoothLocalDevice::AuthorizedPaired) {
		pairingStatusLabel = tr("AUTHORIZED_PAIRED");
		pairingColor = QColor(Qt::blue);
	}

	QString deviceLabel = tr("%1 (%2)   [State: %3]").arg(remoteDeviceInfo.name(),
							      remoteDeviceInfo.address().toString(),
							      pairingStatusLabel);
#endif
	// Create the new item, set its information and add it to the list
	QListWidgetItem *item = new QListWidgetItem(deviceLabel);

	item->setData(Qt::UserRole, QVariant::fromValue(remoteDeviceInfo));
	item->setBackgroundColor(pairingColor);

	ui->discoveredDevicesList->addItem(item);
}

void BtDeviceSelectionDialog::itemClicked(QListWidgetItem *item)
{
	// By default we assume that the devices are paired
	QBluetoothDeviceInfo remoteDeviceInfo = item->data(Qt::UserRole).value<QBluetoothDeviceInfo>();
	QString statusMessage = tr("The device %1 can be used for connection. You can press the Save button.")
				  .arg(remoteDeviceInfo.address().toString());
	bool enableSaveButton = true;

#if !defined(Q_OS_WIN)
	// On other platforms than Windows we can obtain the pairing status so if the devices are not paired we disable the button
	QBluetoothLocalDevice::Pairing pairingStatus = localDevice->pairingStatus(remoteDeviceInfo.address());

	if (pairingStatus == QBluetoothLocalDevice::Unpaired) {
		statusMessage = tr("The device %1 must be paired in order to be used. Please use the context menu for pairing options.")
				  .arg(remoteDeviceInfo.address().toString());
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
		ui->scan->setEnabled(false);
		ui->clear->setEnabled(false);
		return;
	}
#endif
	connect(remoteDeviceDiscoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)),
		this, SLOT(addRemoteDevice(QBluetoothDeviceInfo)));
	connect(remoteDeviceDiscoveryAgent, SIGNAL(finished()),
		this, SLOT(remoteDeviceScanFinished()));
	connect(remoteDeviceDiscoveryAgent, SIGNAL(error(QBluetoothDeviceDiscoveryAgent::Error)),
		this, SLOT(deviceDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error)));
}

#if defined(Q_OS_WIN)
WinBluetoothDeviceDiscoveryAgent::WinBluetoothDeviceDiscoveryAgent(QObject *parent) : QThread(parent)
{
	// Initialize the internal flags by their default values
	running = false;
	stopped = false;
	lastError = QBluetoothDeviceDiscoveryAgent::NoError;
	lastErrorToString = tr("No error");
}

WinBluetoothDeviceDiscoveryAgent::~WinBluetoothDeviceDiscoveryAgent()
{
}

bool WinBluetoothDeviceDiscoveryAgent::isActive() const
{
	return running;
}

QString WinBluetoothDeviceDiscoveryAgent::errorToString() const
{
	return lastErrorToString;
}

QBluetoothDeviceDiscoveryAgent::Error WinBluetoothDeviceDiscoveryAgent::error() const
{
	return lastError;
}

void WinBluetoothDeviceDiscoveryAgent::run()
{
	// Initialize query for device and start the lookup service
	WSAQUERYSET queryset;
	HANDLE hLookup;
	int result = SUCCESS;

	running = true;
	lastError = QBluetoothDeviceDiscoveryAgent::NoError;
	lastErrorToString = tr("No error");

	memset(&queryset, 0, sizeof(WSAQUERYSET));
	queryset.dwSize = sizeof(WSAQUERYSET);
	queryset.dwNameSpace = NS_BTH;

	// The LUP_CONTAINERS flag is used to signal that we are doing a device inquiry
	// while LUP_FLUSHCACHE flag is used to flush the device cache for all inquiries
	// and to do a fresh lookup instead.
	result = WSALookupServiceBegin(&queryset, LUP_CONTAINERS | LUP_FLUSHCACHE, &hLookup);

	if (result != SUCCESS) {
		// Get the last error and emit a signal
		lastErrorToString = qt_error_string();
		lastError = QBluetoothDeviceDiscoveryAgent::PoweredOffError;
		emit error(lastError);

		// Announce that the inquiry finished and restore the stopped flag
		running = false;
		stopped = false;

		return;
	}

	// Declare the necessary variables to collect the information
	BYTE buffer[4096];
	DWORD bufferLength = sizeof(buffer);
	WSAQUERYSET *pResults = (WSAQUERYSET*)&buffer;

	memset(buffer, 0, sizeof(buffer));

	pResults->dwSize = sizeof(WSAQUERYSET);
	pResults->dwNameSpace = NS_BTH;
	pResults->lpBlob = NULL;

	//Start looking for devices
	while (result == SUCCESS && !stopped){
		// LUP_RETURN_NAME and LUP_RETURN_ADDR flags are used to return the name and the address of the discovered device
		result = WSALookupServiceNext(hLookup, LUP_RETURN_NAME | LUP_RETURN_ADDR, &bufferLength, pResults);

		if (result == SUCCESS) {
			// Found a device
			QString deviceAddress(BTH_ADDR_BUF_LEN, Qt::Uninitialized);
			DWORD addressSize = BTH_ADDR_BUF_LEN;

			// Collect the address of the device from the WSAQUERYSET
			SOCKADDR_BTH *socketBthAddress = (SOCKADDR_BTH *) pResults->lpcsaBuffer->RemoteAddr.lpSockaddr;

			// Convert the BTH_ADDR to string
			if (WSAAddressToStringW((LPSOCKADDR) socketBthAddress,
						sizeof (*socketBthAddress),
						NULL,
						reinterpret_cast<wchar_t*>(deviceAddress.data()),
						&addressSize
						) != 0) {
				// Get the last error and emit a signal
				lastErrorToString = qt_error_string();
				lastError = QBluetoothDeviceDiscoveryAgent::UnknownError;
				emit(lastError);

				break;
			}

			// Remove the round parentheses
			deviceAddress.remove(')');
			deviceAddress.remove('(');

			// Save the name of the discovered device and truncate the address
			QString deviceName = QString(pResults->lpszServiceInstanceName);
			deviceAddress.truncate(BTH_ADDR_PRETTY_STRING_LEN);

			// Create an object with information about the discovered device
			QBluetoothDeviceInfo deviceInfo = QBluetoothDeviceInfo(QBluetoothAddress(deviceAddress), deviceName, 0);

			// Raise a signal with information about the found remote device
			emit deviceDiscovered(deviceInfo);
		} else {
			// Get the last error and emit a signal
			lastErrorToString = qt_error_string();
			lastError = QBluetoothDeviceDiscoveryAgent::UnknownError;
			emit(lastError);
		}
	}

	// Announce that the inquiry finished and restore the stopped flag
	running = false;
	stopped = false;

	// Restore the error status
	lastError = QBluetoothDeviceDiscoveryAgent::NoError;

	// End the lookup service
	WSALookupServiceEnd(hLookup);
}

void WinBluetoothDeviceDiscoveryAgent::stop()
{
	// Stop the inqury
	stopped = true;
}
#endif
