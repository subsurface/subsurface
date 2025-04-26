// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/downloadfromdivecomputer.h"
#include "commands/command.h"
#include "core/qthelper.h"
#include "core/device.h"
#include "core/divelist.h"
#include "core/divelog.h"
#include "core/errorhelper.h"
#include "core/settings/qPrefDiveComputer.h"
#include "core/subsurface-float.h"
#include "core/subsurface-string.h"
#include "core/downloadfromdcthread.h"
#include "desktop-widgets/divelistview.h"
#include "desktop-widgets/mainwindow.h"
#include "qt-models/diveimportedmodel.h"
#include "qt-models/models.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QTimer>
#include <QUndoStack>

static bool is_vendor_searchable(QString vendor)
{
	return vendor == "Uemis" || vendor == "Garmin" || vendor == "FIT";
}

DownloadFromDCWidget::DownloadFromDCWidget(const QString &filename, QWidget *parent) : QDialog(parent, QFlag(0)),
	filename(filename),
	downloading(false),
	previousLast(0),
	timer(new QTimer(this)),
	dumpWarningShown(false),
#if defined (BT_SUPPORT)
	btd(nullptr),
#endif
	currentState(INITIAL)
{
	diveImportedModel = new DiveImportedModel(this);
	vendorModel.setStringList(vendorList);
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this);

	int startingWidth = defaultModelFont().pointSize();

	ui.setupUi(this);
	ui.progressBar->hide();
	ui.progressBar->setMinimum(0);
	ui.progressBar->setMaximum(100);
	ui.downloadedView->setModel(diveImportedModel);
	ui.downloadedView->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui.downloadedView->setSelectionMode(QAbstractItemView::SingleSelection);
	ui.downloadedView->setColumnWidth(0, startingWidth * 20);
	ui.downloadedView->setColumnWidth(1, startingWidth * 10);
	ui.downloadedView->setColumnWidth(2, startingWidth * 10);
	ui.chooseDumpFile->setEnabled(ui.dumpToFile->isChecked());
	ui.chooseLogFile->setEnabled(ui.logToFile->isChecked());
	ui.selectAllButton->setEnabled(false);
	ui.unselectAllButton->setEnabled(false);
	ui.vendor->setModel(&vendorModel);
	ui.search->setEnabled(is_vendor_searchable(ui.vendor->currentText()));
	ui.product->setModel(&productModel);
	ui.syncDiveComputerTime->setChecked(prefs.sync_dc_time);

	progress_bar_text.clear();

	timer->setInterval(200);

	connect(ui.downloadedView, SIGNAL(clicked(QModelIndex)), diveImportedModel, SLOT(changeSelected(QModelIndex)));
	connect(ui.chooseDumpFile, SIGNAL(clicked()), this, SLOT(pickDumpFile()));
	connect(ui.dumpToFile, SIGNAL(stateChanged(int)), this, SLOT(checkDumpFile(int)));
	connect(ui.chooseLogFile, SIGNAL(clicked()), this, SLOT(pickLogFile()));
	connect(ui.logToFile, SIGNAL(stateChanged(int)), this, SLOT(checkLogFile(int)));
	connect(ui.selectAllButton, SIGNAL(clicked()), diveImportedModel, SLOT(selectAll()));
	connect(ui.unselectAllButton, SIGNAL(clicked()), diveImportedModel, SLOT(selectNone()));
	connect(ui.syncDiveComputerTime, &QAbstractButton::toggled, qPrefDiveComputer::instance(), &qPrefDiveComputer::set_sync_dc_time);
	connect(timer, SIGNAL(timeout()), this, SLOT(updateProgressBar()));
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));

	connect(diveImportedModel, &DiveImportedModel::downloadFinished, this, &DownloadFromDCWidget::onDownloadThreadFinished);

	if (!qPrefDiveComputer::vendor().isEmpty()) {
		ui.vendor->setCurrentIndex(ui.vendor->findText(qPrefDiveComputer::vendor()));
		productModel.setStringList(productList[qPrefDiveComputer::vendor()]);
		if (!qPrefDiveComputer::product().isEmpty())
			ui.product->setCurrentIndex(ui.product->findText(qPrefDiveComputer::product()));
	}

	// now lets set the four shortcuts for previously used dive computers
	showRememberedDCs();

	updateState(INITIAL);
	ui.ok->setEnabled(false);
	ui.downloadCancelRetryButton->setEnabled(true);
	ui.downloadCancelRetryButton->setText(tr("Download"));

	QString deviceText = qPrefDiveComputer::device();
#if defined(BT_SUPPORT)
	ui.bluetoothMode->setText(tr("Choose Bluetooth download mode"));
	ui.bluetoothMode->setChecked(isBluetoothAddress(qPrefDiveComputer::device()));
	btDeviceSelectionDialog = 0;
	connect(ui.bluetoothMode, SIGNAL(stateChanged(int)), this, SLOT(enableBluetoothMode(int)));
	connect(ui.chooseBluetoothDevice, SIGNAL(clicked()), this, SLOT(selectRemoteBluetoothDevice()));
	ui.chooseBluetoothDevice->setEnabled(ui.bluetoothMode->isChecked());
	if (ui.bluetoothMode->isChecked())
		deviceText = BtDeviceSelectionDialog::formatDeviceText(qPrefDiveComputer::device(), qPrefDiveComputer::device_name());
#else
	ui.bluetoothMode->hide();
	ui.chooseBluetoothDevice->hide();
#endif
	if (!deviceText.isEmpty())
		ui.device->setEditText(deviceText);
}

#define SETUPDC(num) \
	if (!qPrefDiveComputer::vendor##num().isEmpty()) { \
		ui.DC##num->setVisible(true); \
		ui.DCFrame##num->setVisible(true); \
		ui.DeleteDC##num->setVisible(true); \
		ui.DC##num->setText(qPrefDiveComputer::vendor##num() + " - " + qPrefDiveComputer::product##num()); \
		connect(ui.DC##num, &QPushButton::clicked, this, &DownloadFromDCWidget::DC##num##Clicked, Qt::UniqueConnection); \
		connect(ui.DeleteDC##num, &QPushButton::clicked, this, &DownloadFromDCWidget::DeleteDC##num##Clicked, Qt::UniqueConnection); \
	} else { \
		ui.DC##num->setVisible(false); \
		ui.DCFrame##num->setVisible(false); \
		ui.DeleteDC##num->setVisible(false); \
	}

void DownloadFromDCWidget::showRememberedDCs()
{
	SETUPDC(1)
	SETUPDC(2)
	SETUPDC(3)
	SETUPDC(4)
}

int DownloadFromDCWidget::deviceIndex(QString deviceText)
{
	int rv = ui.device->findText(deviceText);
	if (rv == -1) {
		// we need to insert the device text into the model
		ui.device->addItem(deviceText);
		rv = ui.device->findText(deviceText);
	}
	return rv;
}


// DC button slots
// we need two versions as one of the helper functions used is only available if
// Bluetooth support is enabled
#ifdef BT_SUPPORT
#define DCBUTTON(num) \
void DownloadFromDCWidget::DC##num##Clicked() \
{ \
	ui.vendor->setCurrentIndex(ui.vendor->findText(qPrefDiveComputer::vendor##num())); \
	productModel.setStringList(productList[qPrefDiveComputer::vendor##num()]); \
	ui.product->setCurrentIndex(ui.product->findText(qPrefDiveComputer::product##num())); \
	ui.device->setCurrentIndex(deviceIndex(qPrefDiveComputer::device##num())); \
	ui.bluetoothMode->setChecked(isBluetoothAddress(qPrefDiveComputer::device##num())); \
}
#else
#define DCBUTTON(num) \
void DownloadFromDCWidget::DC##num##Clicked() \
{ \
	ui.vendor->setCurrentIndex(ui.vendor->findText(qPrefDiveComputer::vendor##num())); \
	productModel.setStringList(productList[qPrefDiveComputer::vendor##num()]); \
	ui.product->setCurrentIndex(ui.product->findText(qPrefDiveComputer::product##num())); \
	ui.device->setCurrentIndex(deviceIndex(qPrefDiveComputer::device##num())); \
}
#endif

DCBUTTON(1)
DCBUTTON(2)
DCBUTTON(3)
DCBUTTON(4)

// Delete DC button slots
#define DELETEDCBUTTON(num) \
void DownloadFromDCWidget::DeleteDC##num##Clicked() \
{ \
	ui.DC##num->setVisible(false); \
	ui.DeleteDC##num->setVisible(false); \
	int dc = num; \
	switch (dc) { \
		case 1: \
			qPrefDiveComputer::set_vendor1(qPrefDiveComputer::vendor2()); \
			qPrefDiveComputer::set_product1(qPrefDiveComputer::product2()); \
			qPrefDiveComputer::set_device1(qPrefDiveComputer::device2()); \
		case 2: \
			qPrefDiveComputer::set_vendor2(qPrefDiveComputer::vendor3()); \
			qPrefDiveComputer::set_product2(qPrefDiveComputer::product3()); \
			qPrefDiveComputer::set_device2(qPrefDiveComputer::device3()); \
		case 3: \
			qPrefDiveComputer::set_vendor3(qPrefDiveComputer::vendor4()); \
			qPrefDiveComputer::set_product3(qPrefDiveComputer::product4()); \
			qPrefDiveComputer::set_device3(qPrefDiveComputer::device4()); \
		case 4: \
			qPrefDiveComputer::set_vendor4(QString()); \
			qPrefDiveComputer::set_product4(QString()); \
			qPrefDiveComputer::set_device4(QString()); \
	} \
	DownloadFromDCWidget::showRememberedDCs(); \
}

DELETEDCBUTTON(1)
DELETEDCBUTTON(2)
DELETEDCBUTTON(3)
DELETEDCBUTTON(4)

void DownloadFromDCWidget::updateProgressBar()
{
	if (last_text.empty()) {
		// if we get the first actual text after the download is finished
		// (which happens for example on the OSTC), then don't bother
		if (!progress_bar_text.empty() && nearly_equal(progress_bar_fraction, 1.0))
			progress_bar_text.clear();
	}
	if (!progress_bar_text.empty()) {
		// once the progress bar text is set, setup the maximum so the user sees actual progress
		ui.progressBar->setFormat(QString::fromStdString(progress_bar_text));
		ui.progressBar->setMaximum(100);
#if defined(Q_OS_MAC)
		// on mac the progress bar doesn't show its text
		ui.progressText->setText(QString::fromStdString(progress_bar_text));
#endif
	} else {
		if (nearly_0(progress_bar_fraction)) {
			// while we are waiting to connect, set the maximum to 0 so we get a busy indication
			ui.progressBar->setMaximum(0);
			ui.progressBar->setFormat(tr("Connecting to dive computer"));
#if defined(Q_OS_MAC)
			// on mac the progress bar doesn't show its text
			ui.progressText->setText(tr("Connecting to dive computer"));
#endif
		} else {
			// we have some progress - reset the maximum so the user sees actual progress
			ui.progressBar->setMaximum(100);
			ui.progressBar->setFormat("%p%");
#if defined(Q_OS_MAC)
			// on mac the progress bar doesn't show its text
			ui.progressText->setText(QString("%1%").arg(lrint(progress_bar_fraction * 100)));
#endif
		}
	}
	ui.progressBar->setValue(lrint(progress_bar_fraction * 100));
	last_text = progress_bar_text;
}

void DownloadFromDCWidget::checkShowError(states state)
{
	if (!diveImportedModel->thread.error.empty()) {
		if (state == DONE)
			QMessageBox::warning(this, TITLE_OR_TEXT(tr("Warning"), QString::fromStdString(diveImportedModel->thread.error)), QMessageBox::Ok);
		else
			QMessageBox::critical(this, TITLE_OR_TEXT(tr("Error"), QString::fromStdString(diveImportedModel->thread.error)), QMessageBox::Ok);
	}
}

void DownloadFromDCWidget::updateState(states state)
{
	if (state == currentState)
		return;

	if (state == INITIAL) {
		fill_device_list(~0);
		ui.progressBar->hide();
		markChildrenAsEnabled();
		timer->stop();
		progress_bar_text.clear();
#if defined(Q_OS_MAC)
		// on mac we show the text in a label
		ui.progressText->setText(QString::fromStdString(progress_bar_text));
#endif
	}

	// tries to cancel an on going download
	else if (currentState == DOWNLOADING && state == CANCELLING) {
		import_thread_cancelled = true;
		ui.downloadCancelRetryButton->setEnabled(false);
	}

	// user pressed cancel but the application isn't doing anything.
	// means close the window
	else if ((currentState == INITIAL || currentState == DONE || currentState == ERRORED) && state == CANCELLING) {
		timer->stop();
		reject();
	}

	// the cancelation process is finished
	else if (currentState == CANCELLING && state == DONE) {
		timer->stop();
		ui.progressBar->setValue(0);
		ui.progressBar->hide();
		markChildrenAsEnabled();
		progress_bar_text.clear();
#if defined(Q_OS_MAC)
		// on mac we show the text in a label
		ui.progressText->setText(QString::fromStdString(progress_bar_text));
#endif
		checkShowError(state);
	}

	// DOWNLOAD is finally done, but we don't know if there was an error as libdivecomputer doesn't pass
	// that information on to us.
	// If we find an error, offer to retry, otherwise continue the interaction to pick the dives the user wants
	else if (currentState == DOWNLOADING && state == DONE) {
		timer->stop();
		if (QString::fromStdString(progress_bar_text).contains("error", Qt::CaseInsensitive)) {
			updateProgressBar();
			markChildrenAsEnabled();
			progress_bar_text.clear();
		} else {
			if (diveImportedModel->numDives() != 0)
				progress_bar_text.clear();
			ui.progressBar->setValue(100);
			markChildrenAsEnabled();
		}
#if defined(Q_OS_MAC)
		// on mac we show the text in a label
		ui.progressText->setText(QString::fromStdString(progress_bar_text));
#endif
		checkShowError(state);
	}

	// DOWNLOAD is started.
	else if (state == DOWNLOADING) {
		timer->start();
		ui.progressBar->setValue(0);
		progress_bar_fraction = 0.0;
		updateProgressBar();
		ui.progressBar->show();
		markChildrenAsDisabled();
	}

	// got an error
	else if (state == ERRORED) {
		timer->stop();

		checkShowError(state);
		markChildrenAsEnabled();
		progress_bar_text.clear();
		ui.progressBar->hide();
#if defined(Q_OS_MAC)
		// on mac we show the text in a label
		ui.progressText->setText(QString::fromStdString(progress_bar_text));
#endif
	}

	// properly updating the widget state
	currentState = state;
}

void DownloadFromDCWidget::on_vendor_currentTextChanged(const QString &vendor)
{
	unsigned int transport;
	dc_descriptor_t *descriptor;
	productModel.setStringList(productList[vendor]);
	ui.product->setCurrentIndex(0);

	descriptor = descriptorLookup.value(ui.vendor->currentText().toLower() + ui.product->currentText().toLower());
	transport = dc_descriptor_get_transports(descriptor);
	fill_device_list(transport);
	ui.search->setEnabled(is_vendor_searchable(vendor));
}

void DownloadFromDCWidget::on_product_currentTextChanged(const QString &)
{
	updateTransportSelection(true);
}

void DownloadFromDCWidget::on_device_currentTextChanged(const QString &device)
{
#if defined(BT_SUPPORT) && defined(Q_OS_MACOS)
	if (isBluetoothAddress(device)) {
		// ensure we have a discovery running
		if (btd == nullptr)
			btd = BTDiscovery::instance();
		btd->discoverAddress(device);
	}
#else
	Q_UNUSED(device)
#endif
}

void DownloadFromDCWidget::on_search_clicked()
{
	if (is_vendor_searchable(ui.vendor->currentText())) {
		QString dialogTitle;
		if (ui.vendor->currentText() == "Uemis")
			dialogTitle = tr("Find Uemis dive computer");
		else if (ui.vendor->currentText() == "Garmin")
			dialogTitle = tr("Find Garmin dive computer");
		else if (ui.vendor->currentText() == "FIT")
			dialogTitle = tr("Select diretory to import .fit files from");
		QString dirName = QFileDialog::getExistingDirectory(this,
								    dialogTitle,
								    QDir::homePath(),
								    QFileDialog::ShowDirsOnly);
		if (ui.device->findText(dirName) == -1)
			ui.device->addItem(dirName);
		ui.device->setEditText(dirName);
	}
}

void DownloadFromDCWidget::on_downloadCancelRetryButton_clicked()
{
	if (currentState == DOWNLOADING) {
		updateState(CANCELLING);
		return;
	}
	if (currentState == DONE) {
		// this means we are retrying - so we better clean out the partial
		// list of downloaded dives from the last attempt
		diveImportedModel->clearTable();
	}
	updateState(DOWNLOADING);

	// you cannot cancel the dialog, just the download
	ui.cancel->setEnabled(false);
	ui.downloadCancelRetryButton->setText(tr("Cancel download"));

	auto data = diveImportedModel->thread.data();
	data->setVendor(ui.vendor->currentText());
	data->setProduct(ui.product->currentText());
#if defined(BT_SUPPORT)
	data->setBluetoothMode(ui.bluetoothMode->isChecked());
	if (data->bluetoothMode()) {
		// Get the selected device address
		if (btDeviceSelectionDialog != NULL) {
			data->setDevName(btDeviceSelectionDialog->getSelectedDeviceAddress());
			data->setDevBluetoothName(btDeviceSelectionDialog->getSelectedDeviceName());
		} else {
			auto [address, name] = extractBluetoothNameAddress(ui.device->currentText());
			data->setDevName(address);
			data->setDevBluetoothName(name);
		}
		if (btd) {
			// btd should only be active for mac os.
			// Need to ensure that any scan is shut down BEFORE starting the download
			// because internally (as of 5.15.13) the QT discovery agent uses a QTimer
			// that can only be shut off from the same thread it was started from.
			QString devAddr = data->devName().startsWith("LE:")
							 ? data->devName().right(data->devName().size()-3)
							 : data->devName();
			getBtDeviceInfo(devAddr);
		}
	} else
		// this breaks an "else if" across lines... not happy...
#endif
	if (data->vendor() == "Uemis") {
		QString devname = ui.device->currentText();
		auto colon = devname.indexOf(":\\ (UEMISSDA)");
		if (colon >= 0) {
			devname.resize(colon + 2);
			report_info("shortened devname to \"%s\"", qPrintable(devname));
		}
		data->setDevName(devname);
	} else {
		data->setDevName(ui.device->currentText());
	}

	data->setForceDownload(ui.forceDownload->isChecked());
	data->setSaveLog(ui.logToFile->isChecked());
	data->setSaveDump(ui.dumpToFile->isChecked());
	data->setSyncTime(ui.syncDiveComputerTime->isChecked());

	qPrefDiveComputer::set_vendor(data->vendor());
	qPrefDiveComputer::set_product(data->product());
	qPrefDiveComputer::set_device(data->devName());

	// before we start, remember where the dive_table ended
	previousLast = static_cast<int>(divelog.dives.size());
	diveImportedModel->startDownload();

	// FIXME: We should get the _actual_ device info instead of whatever
	// the user entered in the dropdown.
	// You can enter "OSTC 3" and download just fine from a "OSTC Sport", but
	// this check will compair apples and oranges, firmware wise, then.
	QString product(ui.product->currentText());
	//
	// We shouldn't do this for memory dumps.
	if ((product == "OSTC 3" || product == "OSTC 3+" || product == "OSTC cR" ||
	     product == "OSTC Sport" || product == "OSTC 4/5" || product == "OSTC Plus") &&
	    !data->saveDump()) {
		ostcFirmwareCheck.reset(new OstcFirmwareCheck(product));
	}
}

bool DownloadFromDCWidget::preferDownloaded()
{
	return ui.preferDownloaded->isChecked();
}

void DownloadFromDCWidget::checkLogFile(int state)
{
	ui.chooseLogFile->setEnabled(state == Qt::Checked);
	// TODO: Verify the Thread.
	if (state == Qt::Checked) {
		pickLogFile();
	}
}

void DownloadFromDCWidget::pickLogFile()
{
	QFileInfo fi(filename);
	QString logfilename = fi.absolutePath().append(QDir::separator()).append("subsurface.log");
	QString logFile = QFileDialog::getSaveFileName(this, tr("Choose file for dive computer download logfile"),
						       logfilename, tr("Log files") + " (*.log)");
	if (!logFile.isEmpty())
		logfile_name = logFile.toStdString();
}

void DownloadFromDCWidget::checkDumpFile(int state)
{
	ui.chooseDumpFile->setEnabled(state == Qt::Checked);
	if (state == Qt::Checked) {
		pickDumpFile();
		if (!dumpWarningShown) {
			QMessageBox::warning(this, tr("Warning"),
					     tr("Saving the libdivecomputer dump will NOT download dives to the dive list."));
			dumpWarningShown = true;
		}
	}
}

void DownloadFromDCWidget::pickDumpFile()
{
	QFileInfo fi(filename);
	QString dumpfilename = fi.absolutePath().append(QDir::separator()).append("subsurface.bin");
	QString dumpFile = QFileDialog::getSaveFileName(this, tr("Choose file for dive computer binary dump file"),
							dumpfilename, tr("Dump files") + " (*.bin)");
	if (!dumpFile.isEmpty())
		dumpfile_name = dumpFile.toStdString();
}

void DownloadFromDCWidget::reject()
{
	// we don't want the download window being able to close
	// while we're still downloading.
	if (currentState != DOWNLOADING && currentState != CANCELLING)
		QDialog::reject();
}

void DownloadFromDCWidget::onDownloadThreadFinished()
{
	// let's update the remembered DCs
	showRememberedDCs();

	if (currentState == DOWNLOADING) {
		if (diveImportedModel->thread.successful)
			updateState(DONE);
		else
			updateState(ERRORED);
	} else if (currentState == CANCELLING) {
		updateState(DONE);
	}
	ui.downloadCancelRetryButton->setText(tr("Retry download"));
	ui.downloadCancelRetryButton->setEnabled(true);
}

void DownloadFromDCWidget::on_cancel_clicked()
{
	if (currentState == DOWNLOADING || currentState == CANCELLING)
		return;

	// now discard all the dives
	diveImportedModel->clearTable();
	done(-1);
}

void DownloadFromDCWidget::on_ok_clicked()
{
	if (currentState != DONE && currentState != ERRORED)
		return;

	int flags = import_flags::is_downloaded;
	if (preferDownloaded())
		flags |= import_flags::prefer_imported;
	if (ui.createNewTrip->isChecked())
		flags |= import_flags::add_to_new_trip;

	diveImportedModel->recordDives(flags);

	if (ostcFirmwareCheck && currentState == DONE) {
		connect(ostcFirmwareCheck.get(), &OstcFirmwareCheck::checkCompleted, this, &DownloadFromDCWidget::accept);
		ostcFirmwareCheck->checkLatest(this, diveImportedModel->thread.data()->internalData(), filename);
	} else {
		accept();
	}
}

void DownloadFromDCWidget::updateTransportSelection(bool changeSelection)
{
	unsigned int transports = dc_descriptor_get_transports(descriptorLookup.value(ui.vendor->currentText().toLower() + ui.product->currentText().toLower()));

	// call dc_descriptor_get_transport to see if the dc_transport_t is DC_TRANSPORT_SERIAL
	if (transports & (DC_TRANSPORT_SERIAL | DC_TRANSPORT_USBSTORAGE)) {
		// if the dc_transport_t is DC_TRANSPORT_SERIAL, then enable the device node box.
		ui.device->setEnabled(true);
	} else {
		// otherwise disable the device node box
		ui.device->setEnabled(false);
	}

#if defined(BT_SUPPORT)
	if (transports & (DC_TRANSPORT_BLUETOOTH | DC_TRANSPORT_BLE)) {
		unsigned int nonBluetoothTransports = transports & ~(DC_TRANSPORT_BLUETOOTH | DC_TRANSPORT_BLE);
		if (!nonBluetoothTransports) {
			ui.bluetoothMode->setChecked(true);
			ui.bluetoothMode->setEnabled(false);
			ui.chooseBluetoothDevice->setEnabled(true);
		} else {
			if (changeSelection && nonBluetoothTransports == DC_TRANSPORT_SERIAL)
				// only bluetooth and (virtual bluetooth) serial port are supported - prefer bluetooth
				ui.bluetoothMode->setChecked(true);

			ui.bluetoothMode->setEnabled(true);
			ui.chooseBluetoothDevice->setEnabled(true);
		}
	} else {
		ui.bluetoothMode->setChecked(false);
		ui.bluetoothMode->setEnabled(false);
		ui.chooseBluetoothDevice->setEnabled(false);
	}
#endif
}

void DownloadFromDCWidget::markChildrenAsDisabled()
{
	ui.device->setEnabled(false);
	ui.vendor->setEnabled(false);
	ui.product->setEnabled(false);
	ui.forceDownload->setEnabled(false);
	ui.createNewTrip->setEnabled(false);
	ui.preferDownloaded->setEnabled(false);
	ui.ok->setEnabled(false);
	ui.search->setEnabled(false);
	ui.logToFile->setEnabled(false);
	ui.dumpToFile->setEnabled(false);
	ui.chooseLogFile->setEnabled(false);
	ui.chooseDumpFile->setEnabled(false);
	ui.selectAllButton->setEnabled(false);
	ui.unselectAllButton->setEnabled(false);
	ui.bluetoothMode->setEnabled(false);
	ui.chooseBluetoothDevice->setEnabled(false);
	ui.syncDiveComputerTime->setEnabled(false);
}

void DownloadFromDCWidget::markChildrenAsEnabled()
{
	updateTransportSelection(false);
	ui.vendor->setEnabled(true);
	ui.product->setEnabled(true);
	ui.forceDownload->setEnabled(true);
	ui.createNewTrip->setEnabled(true);
	ui.preferDownloaded->setEnabled(true);
	ui.ok->setEnabled(true);
	ui.cancel->setEnabled(true);
	ui.search->setEnabled(true);
	ui.logToFile->setEnabled(true);
	ui.dumpToFile->setEnabled(true);
	ui.chooseLogFile->setEnabled(true);
	ui.chooseDumpFile->setEnabled(true);
	ui.selectAllButton->setEnabled(true);
	ui.unselectAllButton->setEnabled(true);
	ui.syncDiveComputerTime->setEnabled(true);
}

#if defined(BT_SUPPORT)
void DownloadFromDCWidget::selectRemoteBluetoothDevice()
{
	if (!btDeviceSelectionDialog) {
		btDeviceSelectionDialog = new BtDeviceSelectionDialog(this);
		connect(btDeviceSelectionDialog, SIGNAL(finished(int)),
			this, SLOT(bluetoothSelectionDialogIsFinished(int)));
	}

	btDeviceSelectionDialog->show();
}

void DownloadFromDCWidget::bluetoothSelectionDialogIsFinished(int result)
{
	if (result == QDialog::Accepted) {
		/* Make the selected Bluetooth device default */
		ui.device->setEditText(btDeviceSelectionDialog->getSelectedDeviceText());
	} else if (result == QDialog::Rejected) {
		updateTransportSelection(false);
	}
}

void DownloadFromDCWidget::enableBluetoothMode(int state)
{
	ui.chooseBluetoothDevice->setEnabled(state == Qt::Checked);

	/*	This is convoluted enough to warrant explanation:
 		1. If Bluetooth is enabled, but no Bluetooth address then scan.
		2. If Bluetooth is enabled and we have a Bluetooth address, skip scan.
		3. If Bluetooth is not enabled, but it's a Bluetooth address, clear it. */
	if (state == Qt::Checked) {
		if (!isBluetoothAddress(ui.device->currentText()))
			selectRemoteBluetoothDevice();
	} else
		if (isBluetoothAddress(ui.device->currentText()))
			ui.device->setCurrentIndex(-1);
}
#endif

static void fillDeviceList(const char *name, void *data)
{
	QComboBox *comboBox = (QComboBox *)data;
	comboBox->addItem(name);
}

void DownloadFromDCWidget::fill_device_list(unsigned int transport)
{
	int deviceIndex;
	ui.device->clear();
	deviceIndex = enumerate_devices(fillDeviceList, ui.device, transport);
	if (deviceIndex >= 0)
		ui.device->setCurrentIndex(deviceIndex);
}
