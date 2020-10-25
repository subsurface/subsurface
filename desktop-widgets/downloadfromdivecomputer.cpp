// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/downloadfromdivecomputer.h"
#include "commands/command.h"
#include "core/display.h"
#include "core/qthelper.h"
#include "core/divelist.h"
#include "core/settings/qPrefDiveComputer.h"
#include "core/subsurface-string.h"
#include "core/uemis.h"
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

DownloadFromDCWidget::DownloadFromDCWidget(QWidget *parent) : QDialog(parent, QFlag(0)),
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
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);

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
	ui.product->setModel(&productModel);

	progress_bar_text = "";

	timer->setInterval(200);

	connect(ui.downloadedView, SIGNAL(clicked(QModelIndex)), diveImportedModel, SLOT(changeSelected(QModelIndex)));
	connect(ui.chooseDumpFile, SIGNAL(clicked()), this, SLOT(pickDumpFile()));
	connect(ui.dumpToFile, SIGNAL(stateChanged(int)), this, SLOT(checkDumpFile(int)));
	connect(ui.chooseLogFile, SIGNAL(clicked()), this, SLOT(pickLogFile()));
	connect(ui.logToFile, SIGNAL(stateChanged(int)), this, SLOT(checkLogFile(int)));
	connect(ui.selectAllButton, SIGNAL(clicked()), diveImportedModel, SLOT(selectAll()));
	connect(ui.unselectAllButton, SIGNAL(clicked()), diveImportedModel, SLOT(selectNone()));
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
		ui.DC##num->setText(qPrefDiveComputer::vendor##num() + " - " + qPrefDiveComputer::product##num()); \
		connect(ui.DC##num, &QPushButton::clicked, this, &DownloadFromDCWidget::DC##num##Clicked, Qt::UniqueConnection); \
	} else { \
		ui.DC##num->setVisible(false); \
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
	ui.bluetoothMode->setChecked(isBluetoothAddress(qPrefDiveComputer::device##num())); \
	if (ui.device->currentIndex() == -1) \
		ui.device->setCurrentIndex(deviceIndex(qPrefDiveComputer::device##num())); \
	if (QSysInfo::kernelType() == "darwin") { \
		/* it makes no sense that this would be needed on macOS but not Linux */ \
		QCoreApplication::processEvents(); \
		ui.vendor->update(); \
		ui.product->update(); \
		ui.device->update(); \
	} \
}
#else
#define DCBUTTON(num) \
void DownloadFromDCWidget::DC##num##Clicked() \
{ \
	ui.vendor->setCurrentIndex(ui.vendor->findText(qPrefDiveComputer::vendor##num())); \
	productModel.setStringList(productList[qPrefDiveComputer::vendor##num()]); \
	ui.product->setCurrentIndex(ui.product->findText(qPrefDiveComputer::product##num())); \
	ui.device->setCurrentIndex(deviceIndex(qPrefDiveComputer::device##num())); \
	if (QSysInfo::kernelType() == "darwin") { \
		/* it makes no sense that this would be needed on macOS but not Linux */ \
		QCoreApplication::processEvents(); \
		ui.vendor->update(); \
		ui.product->update(); \
		ui.device->update(); \
	} \
}
#endif



DCBUTTON(1)
DCBUTTON(2)
DCBUTTON(3)
DCBUTTON(4)

void DownloadFromDCWidget::updateProgressBar()
{
	static char *last_text = NULL;

	if (empty_string(last_text)) {
		// if we get the first actual text after the download is finished
		// (which happens for example on the OSTC), then don't bother
		if (!empty_string(progress_bar_text) && IS_FP_SAME(progress_bar_fraction, 1.0))
			progress_bar_text = "";
	}
	if (!empty_string(progress_bar_text)) {
		// once the progress bar text is set, setup the maximum so the user sees actual progress
		ui.progressBar->setFormat(progress_bar_text);
		ui.progressBar->setMaximum(100);
#if defined(Q_OS_MAC)
		// on mac the progress bar doesn't show its text
		ui.progressText->setText(progress_bar_text);
#endif
	} else {
		if (IS_FP_SAME(progress_bar_fraction, 0.0)) {
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
	free(last_text);
	last_text = strdup(progress_bar_text);
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
		progress_bar_text = "";
#if defined(Q_OS_MAC)
		// on mac we show the text in a label
		ui.progressText->setText(progress_bar_text);
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
		progress_bar_text = "";
#if defined(Q_OS_MAC)
		// on mac we show the text in a label
		ui.progressText->setText(progress_bar_text);
#endif
	}

	// DOWNLOAD is finally done, but we don't know if there was an error as libdivecomputer doesn't pass
	// that information on to us.
	// If we find an error, offer to retry, otherwise continue the interaction to pick the dives the user wants
	else if (currentState == DOWNLOADING && state == DONE) {
		timer->stop();
		if (QString(progress_bar_text).contains("error", Qt::CaseInsensitive)) {
			updateProgressBar();
			markChildrenAsEnabled();
			progress_bar_text = "";
		} else {
			if (diveImportedModel->numDives() != 0)
				progress_bar_text = "";
			ui.progressBar->setValue(100);
			markChildrenAsEnabled();
		}
#if defined(Q_OS_MAC)
		// on mac we show the text in a label
		ui.progressText->setText(progress_bar_text);
#endif
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

		QMessageBox::critical(this, TITLE_OR_TEXT(tr("Error"), diveImportedModel->thread.error), QMessageBox::Ok);
		markChildrenAsEnabled();
		progress_bar_text = "";
		ui.progressBar->hide();
#if defined(Q_OS_MAC)
		// on mac we show the text in a label
		ui.progressText->setText(progress_bar_text);
#endif
	}

	// properly updating the widget state
	currentState = state;
}

void DownloadFromDCWidget::on_vendor_currentIndexChanged(const QString &vendor)
{
	unsigned int transport;
	dc_descriptor_t *descriptor;
	productModel.setStringList(productList[vendor]);
	ui.product->setCurrentIndex(0);

	descriptor = descriptorLookup.value(ui.vendor->currentText().toLower() + ui.product->currentText().toLower());
	transport = dc_descriptor_get_transports(descriptor);
	fill_device_list(transport);
}

void DownloadFromDCWidget::on_product_currentIndexChanged(const QString &)
{
	updateDeviceEnabled();
}

void DownloadFromDCWidget::on_device_currentTextChanged(const QString &device)
{
#if defined(Q_OS_MACOS)
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
	if (ui.vendor->currentText() == "Uemis" || ui.vendor->currentText() == "Garmin") {
		QString dialogTitle = ui.vendor->currentText() == "Uemis" ?
					tr("Find Uemis dive computer") : tr("Find Garmin dive computer");
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
			QString name, address;
			address = extractBluetoothNameAddress(ui.device->currentText(), name);
			data->setDevName(address);
			data->setDevBluetoothName(name);
		}
	} else
		// this breaks an "else if" across lines... not happy...
#endif
	if (data->vendor() == "Uemis") {
		char *colon;
		char *devname = copy_qstring(ui.device->currentText());

		if ((colon = strstr(devname, ":\\ (UEMISSDA)")) != NULL) {
			*(colon + 2) = '\0';
			fprintf(stderr, "shortened devname to \"%s\"", devname);
		}
		data->setDevName(devname);
	} else {
		data->setDevName(ui.device->currentText());
	}

	data->setForceDownload(ui.forceDownload->isChecked());
	data->setSaveLog(ui.logToFile->isChecked());
	data->setSaveDump(ui.dumpToFile->isChecked());

	qPrefDiveComputer::set_vendor(data->vendor());
	qPrefDiveComputer::set_product(data->product());
	qPrefDiveComputer::set_device(data->devName());

	// before we start, remember where the dive_table ended
	previousLast = dive_table.nr;
	diveImportedModel->startDownload();

	// FIXME: We should get the _actual_ device info instead of whatever
	// the user entered in the dropdown.
	// You can enter "OSTC 3" and download just fine from a "OSTC Sport", but
	// this check will compair apples and oranges, firmware wise, then.
	QString product(ui.product->currentText());
	//
	// We shouldn't do this for memory dumps.
	if ((product == "OSTC 3" || product == "OSTC 3+" || product == "OSTC cR" ||
	     product == "OSTC Sport" || product == "OSTC 4" || product == "OSTC Plus") &&
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
	QString filename = existing_filename ?: prefs.default_filename;
	QFileInfo fi(filename);
	filename = fi.absolutePath().append(QDir::separator()).append("subsurface.log");
	QString logFile = QFileDialog::getSaveFileName(this, tr("Choose file for dive computer download logfile"),
						       filename, tr("Log files") + " (*.log)");
	if (!logFile.isEmpty()) {
		free(logfile_name);
		logfile_name = copy_qstring(logFile);
	}
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
	QString filename = existing_filename ?: prefs.default_filename;
	QFileInfo fi(filename);
	filename = fi.absolutePath().append(QDir::separator()).append("subsurface.bin");
	QString dumpFile = QFileDialog::getSaveFileName(this, tr("Choose file for dive computer binary dump file"),
							filename, tr("Dump files") + " (*.bin)");
	if (!dumpFile.isEmpty()) {
		free(dumpfile_name);
		dumpfile_name = copy_qstring(dumpFile);
	}
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
		if (diveImportedModel->thread.error.isEmpty())
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

	int flags = IMPORT_IS_DOWNLOADED;
	if (preferDownloaded())
		flags |= IMPORT_PREFER_IMPORTED;
	if (ui.createNewTrip->isChecked())
		flags |= IMPORT_ADD_TO_NEW_TRIP;

	diveImportedModel->recordDives(flags);

	if (ostcFirmwareCheck && currentState == DONE)
		ostcFirmwareCheck->checkLatest(this, diveImportedModel->thread.data()->internalData());
	accept();
}

void DownloadFromDCWidget::updateDeviceEnabled()
{
	// Set up the DC descriptor
	dc_descriptor_t *descriptor = NULL;
	descriptor = descriptorLookup.value(ui.vendor->currentText().toLower() + ui.product->currentText().toLower());

	// call dc_descriptor_get_transport to see if the dc_transport_t is DC_TRANSPORT_SERIAL
	if (dc_descriptor_get_transports(descriptor) & (DC_TRANSPORT_SERIAL | DC_TRANSPORT_USBSTORAGE)) {
		// if the dc_transport_t is DC_TRANSPORT_SERIAL, then enable the device node box.
		ui.device->setEnabled(true);
	} else {
		// otherwise disable the device node box
		ui.device->setEnabled(false);
	}
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
}

void DownloadFromDCWidget::markChildrenAsEnabled()
{
	updateDeviceEnabled();
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
#if defined(BT_SUPPORT)
	ui.bluetoothMode->setEnabled(true);
	ui.chooseBluetoothDevice->setEnabled(true);
#endif
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
		/* Disable Bluetooth download mode */
		ui.bluetoothMode->setChecked(false);
	}
}

void DownloadFromDCWidget::enableBluetoothMode(int state)
{
	ui.chooseBluetoothDevice->setEnabled(state == Qt::Checked);

	if (state == Qt::Checked)
		selectRemoteBluetoothDevice();
	else
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
