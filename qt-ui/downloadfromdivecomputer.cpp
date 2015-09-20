#include "downloadfromdivecomputer.h"
#include "helpers.h"
#include "mainwindow.h"
#include "divelistview.h"
#include "display.h"
#include "uemis.h"
#include "models.h"

#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>

struct product {
	const char *product;
	dc_descriptor_t *descriptor;
	struct product *next;
};

struct vendor {
	const char *vendor;
	struct product *productlist;
	struct vendor *next;
};

struct mydescriptor {
	const char *vendor;
	const char *product;
	dc_family_t type;
	unsigned int model;
};

namespace DownloadFromDcGlobal {
	const char *err_string;
};

struct dive_table downloadTable;

DownloadFromDCWidget::DownloadFromDCWidget(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f),
	thread(0),
	downloading(false),
	previousLast(0),
	vendorModel(0),
	productModel(0),
	timer(new QTimer(this)),
	dumpWarningShown(false),
	ostcFirmwareCheck(0),
	currentState(INITIAL)
{
	clear_table(&downloadTable);
	ui.setupUi(this);
	ui.progressBar->hide();
	ui.progressBar->setMinimum(0);
	ui.progressBar->setMaximum(100);
	diveImportedModel = new DiveImportedModel(this);
	ui.downloadedView->setModel(diveImportedModel);
	ui.downloadedView->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui.downloadedView->setSelectionMode(QAbstractItemView::SingleSelection);
	int startingWidth = defaultModelFont().pointSize();
	ui.downloadedView->setColumnWidth(0, startingWidth * 20);
	ui.downloadedView->setColumnWidth(1, startingWidth * 10);
	ui.downloadedView->setColumnWidth(2, startingWidth * 10);
	connect(ui.downloadedView, SIGNAL(clicked(QModelIndex)), diveImportedModel, SLOT(changeSelected(QModelIndex)));

	progress_bar_text = "";

	fill_computer_list();

	ui.chooseDumpFile->setEnabled(ui.dumpToFile->isChecked());
	connect(ui.chooseDumpFile, SIGNAL(clicked()), this, SLOT(pickDumpFile()));
	connect(ui.dumpToFile, SIGNAL(stateChanged(int)), this, SLOT(checkDumpFile(int)));
	ui.chooseLogFile->setEnabled(ui.logToFile->isChecked());
	connect(ui.chooseLogFile, SIGNAL(clicked()), this, SLOT(pickLogFile()));
	connect(ui.logToFile, SIGNAL(stateChanged(int)), this, SLOT(checkLogFile(int)));
	ui.selectAllButton->setEnabled(false);
	ui.unselectAllButton->setEnabled(false);
	connect(ui.selectAllButton, SIGNAL(clicked()), diveImportedModel, SLOT(selectAll()));
	connect(ui.unselectAllButton, SIGNAL(clicked()), diveImportedModel, SLOT(selectNone()));
	vendorModel = new QStringListModel(vendorList);
	ui.vendor->setModel(vendorModel);
	if (default_dive_computer_vendor) {
		ui.vendor->setCurrentIndex(ui.vendor->findText(default_dive_computer_vendor));
		productModel = new QStringListModel(productList[default_dive_computer_vendor]);
		ui.product->setModel(productModel);
		if (default_dive_computer_product)
			ui.product->setCurrentIndex(ui.product->findText(default_dive_computer_product));
	}
	if (default_dive_computer_device)
		ui.device->setEditText(default_dive_computer_device);

	timer->setInterval(200);
	connect(timer, SIGNAL(timeout()), this, SLOT(updateProgressBar()));
	updateState(INITIAL);
	memset(&data, 0, sizeof(data));
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
	ui.ok->setEnabled(false);
	ui.downloadCancelRetryButton->setEnabled(true);
	ui.downloadCancelRetryButton->setText(tr("Download"));

#if defined(BT_SUPPORT) && defined(SSRF_CUSTOM_SERIAL)
	ui.bluetoothMode->setText(tr("Choose Bluetooth download mode"));
	ui.bluetoothMode->setChecked(default_dive_computer_download_mode == DC_TRANSPORT_BLUETOOTH);
	btDeviceSelectionDialog = 0;
	ui.chooseBluetoothDevice->setEnabled(ui.bluetoothMode->isChecked());
	connect(ui.bluetoothMode, SIGNAL(stateChanged(int)), this, SLOT(enableBluetoothMode(int)));
	connect(ui.chooseBluetoothDevice, SIGNAL(clicked()), this, SLOT(selectRemoteBluetoothDevice()));
#else
	ui.bluetoothMode->hide();
	ui.chooseBluetoothDevice->hide();
#endif
}

void DownloadFromDCWidget::updateProgressBar()
{
	if (*progress_bar_text != '\0') {
		ui.progressBar->setFormat(progress_bar_text);
	} else {
		ui.progressBar->setFormat("%p%");
	}
	ui.progressBar->setValue(progress_bar_fraction * 100);
}

void DownloadFromDCWidget::updateState(states state)
{
	if (state == currentState)
		return;

	if (state == INITIAL) {
		fill_device_list(DC_TYPE_OTHER);
		ui.progressBar->hide();
		markChildrenAsEnabled();
		timer->stop();
	}

	// tries to cancel an on going download
	else if (currentState == DOWNLOADING && state == CANCELLING) {
		import_thread_cancelled = true;
		ui.downloadCancelRetryButton->setEnabled(false);
	}

	// user pressed cancel but the application isn't doing anything.
	// means close the window
	else if ((currentState == INITIAL || currentState == DONE || currentState == ERROR) && state == CANCELLING) {
		timer->stop();
		reject();
	}

	// the cancelation process is finished
	else if (currentState == CANCELLING && state == DONE) {
		timer->stop();
		ui.progressBar->setValue(0);
		ui.progressBar->hide();
		markChildrenAsEnabled();
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
			ui.progressBar->setValue(100);
			markChildrenAsEnabled();
		}
	}

	// DOWNLOAD is started.
	else if (state == DOWNLOADING) {
		timer->start();
		ui.progressBar->setValue(0);
		updateProgressBar();
		ui.progressBar->show();
		markChildrenAsDisabled();
	}

	// got an error
	else if (state == ERROR) {
		QMessageBox::critical(this, TITLE_OR_TEXT(tr("Error"), this->thread->error), QMessageBox::Ok);

		markChildrenAsEnabled();
		ui.progressBar->hide();
	}

	// properly updating the widget state
	currentState = state;
}

void DownloadFromDCWidget::on_vendor_currentIndexChanged(const QString &vendor)
{
	int dcType = DC_TYPE_SERIAL;
	QAbstractItemModel *currentModel = ui.product->model();
	if (!currentModel)
		return;

	productModel = new QStringListModel(productList[vendor]);
	ui.product->setModel(productModel);

	if (vendor == QString("Uemis"))
		dcType = DC_TYPE_UEMIS;
	fill_device_list(dcType);

	// Memleak - but deleting gives me a crash.
	//currentModel->deleteLater();
}

void DownloadFromDCWidget::on_product_currentIndexChanged(const QString &product)
{
	// Set up the DC descriptor
	dc_descriptor_t *descriptor = NULL;
	descriptor = descriptorLookup[ui.vendor->currentText() + product];

	// call dc_descriptor_get_transport to see if the dc_transport_t is DC_TRANSPORT_SERIAL
	if (dc_descriptor_get_transport(descriptor) == DC_TRANSPORT_SERIAL) {
		// if the dc_transport_t is DC_TRANSPORT_SERIAL, then enable the device node box.
		ui.device->setEnabled(true);
	} else {
		// otherwise disable the device node box
		ui.device->setEnabled(false);
	}
}

void DownloadFromDCWidget::fill_computer_list()
{
	dc_iterator_t *iterator = NULL;
	dc_descriptor_t *descriptor = NULL;
	struct mydescriptor *mydescriptor;

	QStringList computer;
	dc_descriptor_iterator(&iterator);
	while (dc_iterator_next(iterator, &descriptor) == DC_STATUS_SUCCESS) {
		const char *vendor = dc_descriptor_get_vendor(descriptor);
		const char *product = dc_descriptor_get_product(descriptor);

		if (!vendorList.contains(vendor))
			vendorList.append(vendor);

		if (!productList[vendor].contains(product))
			productList[vendor].push_back(product);

		descriptorLookup[QString(vendor) + QString(product)] = descriptor;
	}
	dc_iterator_free(iterator);
	Q_FOREACH (QString vendor, vendorList)
		qSort(productList[vendor]);

	/* and add the Uemis Zurich which we are handling internally
	   THIS IS A HACK as we magically have a data structure here that
	   happens to match a data structure that is internal to libdivecomputer;
	   this WILL BREAK if libdivecomputer changes the dc_descriptor struct...
	   eventually the UEMIS code needs to move into libdivecomputer, I guess */

	mydescriptor = (struct mydescriptor *)malloc(sizeof(struct mydescriptor));
	mydescriptor->vendor = "Uemis";
	mydescriptor->product = "Zurich";
	mydescriptor->type = DC_FAMILY_NULL;
	mydescriptor->model = 0;

	if (!vendorList.contains("Uemis"))
		vendorList.append("Uemis");

	if (!productList["Uemis"].contains("Zurich"))
		productList["Uemis"].push_back("Zurich");

	descriptorLookup["UemisZurich"] = (dc_descriptor_t *)mydescriptor;

	qSort(vendorList);
}

void DownloadFromDCWidget::on_search_clicked()
{
	if (ui.vendor->currentText() == "Uemis") {
		QString dirName = QFileDialog::getExistingDirectory(this,
								    tr("Find Uemis dive computer"),
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
		clear_table(&downloadTable);
	}
	updateState(DOWNLOADING);

	// you cannot cancel the dialog, just the download
	ui.cancel->setEnabled(false);
	ui.downloadCancelRetryButton->setText("Cancel download");

	// I don't really think that create/destroy the thread
	// is really necessary.
	if (thread) {
		thread->deleteLater();
	}

	data.vendor = strdup(ui.vendor->currentText().toUtf8().data());
	data.product = strdup(ui.product->currentText().toUtf8().data());
#if defined(BT_SUPPORT)
	data.bluetooth_mode = ui.bluetoothMode->isChecked();
	if (data.bluetooth_mode && btDeviceSelectionDialog != NULL) {
		// Get the selected device address
		data.devname = strdup(btDeviceSelectionDialog->getSelectedDeviceAddress().toUtf8().data());
	} else
		// this breaks an "else if" across lines... not happy...
#endif
	if (same_string(data.vendor, "Uemis")) {
		char *colon;
		char *devname = strdup(ui.device->currentText().toUtf8().data());

		if ((colon = strstr(devname, ":\\ (UEMISSDA)")) != NULL) {
			*(colon + 2) = '\0';
			fprintf(stderr, "shortened devname to \"%s\"", data.devname);
		}
		data.devname = devname;
	} else {
		data.devname = strdup(ui.device->currentText().toUtf8().data());
	}
	data.descriptor = descriptorLookup[ui.vendor->currentText() + ui.product->currentText()];
	data.force_download = ui.forceDownload->isChecked();
	data.create_new_trip = ui.createNewTrip->isChecked();
	data.trip = NULL;
	data.deviceid = data.diveid = 0;
	set_default_dive_computer(data.vendor, data.product);
	set_default_dive_computer_device(data.devname);
#if defined(BT_SUPPORT) && defined(SSRF_CUSTOM_SERIAL)
	set_default_dive_computer_download_mode(ui.bluetoothMode->isChecked() ? DC_TRANSPORT_BLUETOOTH : DC_TRANSPORT_SERIAL);
#endif
	thread = new DownloadThread(this, &data);

	connect(thread, SIGNAL(finished()),
		this, SLOT(onDownloadThreadFinished()), Qt::QueuedConnection);

	MainWindow *w = MainWindow::instance();
	connect(thread, SIGNAL(finished()), w, SLOT(refreshDisplay()));

	// before we start, remember where the dive_table ended
	previousLast = dive_table.nr;

	thread->start();

	QString product(ui.product->currentText());
	if (product == "OSTC 3" || product == "OSTC Sport")
		ostcFirmwareCheck = new OstcFirmwareCheck(product);
}

bool DownloadFromDCWidget::preferDownloaded()
{
	return ui.preferDownloaded->isChecked();
}

void DownloadFromDCWidget::checkLogFile(int state)
{
	ui.chooseLogFile->setEnabled(state == Qt::Checked);
	data.libdc_log = (state == Qt::Checked);
	if (state == Qt::Checked && logFile.isEmpty()) {
		pickLogFile();
	}
}

void DownloadFromDCWidget::pickLogFile()
{
	QString filename = existing_filename ?: prefs.default_filename;
	QFileInfo fi(filename);
	filename = fi.absolutePath().append(QDir::separator()).append("subsurface.log");
	logFile = QFileDialog::getSaveFileName(this, tr("Choose file for divecomputer download logfile"),
					       filename, tr("Log files (*.log)"));
	if (!logFile.isEmpty()) {
		free(logfile_name);
		logfile_name = strdup(logFile.toUtf8().data());
	}
}

void DownloadFromDCWidget::checkDumpFile(int state)
{
	ui.chooseDumpFile->setEnabled(state == Qt::Checked);
	data.libdc_dump = (state == Qt::Checked);
	if (state == Qt::Checked) {
		if (dumpFile.isEmpty())
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
	dumpFile = QFileDialog::getSaveFileName(this, tr("Choose file for divecomputer binary dump file"),
						filename, tr("Dump files (*.bin)"));
	if (!dumpFile.isEmpty()) {
		free(dumpfile_name);
		dumpfile_name = strdup(dumpFile.toUtf8().data());
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
	if (currentState == DOWNLOADING) {
		if (thread->error.isEmpty())
			updateState(DONE);
		else
			updateState(ERROR);
	} else if (currentState == CANCELLING) {
		updateState(DONE);
	}
	ui.downloadCancelRetryButton->setText(tr("Retry"));
	ui.downloadCancelRetryButton->setEnabled(true);
	// regardless, if we got dives, we should show them to the user
	if (downloadTable.nr) {
		diveImportedModel->setImportedDivesIndexes(0, downloadTable.nr - 1);
	}

}

void DownloadFromDCWidget::on_cancel_clicked()
{
	if (currentState == DOWNLOADING || currentState == CANCELLING)
		return;

	// now discard all the dives
	clear_table(&downloadTable);
	done(-1);
}

void DownloadFromDCWidget::on_ok_clicked()
{
	struct dive *dive;

	if (currentState != DONE && currentState != ERROR)
		return;

	// record all the dives in the 'real' dive_table
	for (int i = 0; i < downloadTable.nr; i++) {
		if (diveImportedModel->data(diveImportedModel->index(i, 0),Qt::CheckStateRole) == Qt::Checked)
			record_dive(downloadTable.dives[i]);
		downloadTable.dives[i] = NULL;
	}
	downloadTable.nr = 0;

	int uniqId, idx;
	// remember the last downloaded dive (on most dive computers this will be the chronologically
	// first new dive) and select it again after processing all the dives
	MainWindow::instance()->dive_list()->unselectDives();

	dive = get_dive(dive_table.nr - 1);
	if (dive != NULL) {
		uniqId = get_dive(dive_table.nr - 1)->id;
		process_dives(true, preferDownloaded());
		// after process_dives does any merging or resorting needed, we need
		// to recreate the model for the dive list so we can select the newest dive
		MainWindow::instance()->recreateDiveList();
		idx = get_idx_by_uniq_id(uniqId);
		// this shouldn't be necessary - but there are reports that somehow existing dives stay selected
		// (but not visible as selected)
		MainWindow::instance()->dive_list()->unselectDives();
		MainWindow::instance()->dive_list()->selectDive(idx, true);
	}

	if (ostcFirmwareCheck && currentState == DONE)
		ostcFirmwareCheck->checkLatest(this, &data);
	accept();
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
	ui.device->setEnabled(true);
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
		QString selectedDeviceName = btDeviceSelectionDialog->getSelectedDeviceName();

		if (selectedDeviceName == NULL || selectedDeviceName.isEmpty()) {
			ui.device->setCurrentText(btDeviceSelectionDialog->getSelectedDeviceAddress());
		} else {
			ui.device->setCurrentText(selectedDeviceName);
		}
	} else if (result == QDialog::Rejected){
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

void DownloadFromDCWidget::fill_device_list(int dc_type)
{
	int deviceIndex;
	ui.device->clear();
	deviceIndex = enumerate_devices(fillDeviceList, ui.device, dc_type);
	if (deviceIndex >= 0)
		ui.device->setCurrentIndex(deviceIndex);
}

DownloadThread::DownloadThread(QObject *parent, device_data_t *data) : QThread(parent),
	data(data)
{
}

static QString str_error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	const QString str = QString().vsprintf(fmt, args);
	va_end(args);

	return str;
}

void DownloadThread::run()
{
	const char *errorText;
	import_thread_cancelled = false;
	data->download_table = &downloadTable;
	if (!strcmp(data->vendor, "Uemis"))
		errorText = do_uemis_import(data);
	else
		errorText = do_libdivecomputer_import(data);
	if (errorText)
		error = str_error(errorText, data->devname, data->vendor, data->product);
}

DiveImportedModel::DiveImportedModel(QObject *o) : QAbstractTableModel(o),
	firstIndex(0),
	lastIndex(-1),
	checkStates(0)
{
}

int DiveImportedModel::columnCount(const QModelIndex &model) const
{
	return 3;
}

int DiveImportedModel::rowCount(const QModelIndex &model) const
{
	return lastIndex - firstIndex + 1;
}

QVariant DiveImportedModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Vertical)
		return QVariant();
	if (role == Qt::DisplayRole) {
		switch (section) {
		case 0:
			return QVariant(tr("Date/time"));
		case 1:
			return QVariant(tr("Duration"));
		case 2:
			return QVariant(tr("Depth"));
		}
	}
	return QVariant();
}

QVariant DiveImportedModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (index.row() + firstIndex > lastIndex)
		return QVariant();

	struct dive *d = get_dive_from_table(index.row() + firstIndex, &downloadTable);
	if (!d)
		return QVariant();
	if (role == Qt::DisplayRole) {
		switch (index.column()) {
		case 0:
			return QVariant(get_short_dive_date_string(d->when));
		case 1:
			return QVariant(get_dive_duration_string(d->duration.seconds, tr("h:"), tr("min")));
		case 2:
			return QVariant(get_depth_string(d->maxdepth.mm, true, false));
		}
	}
	if (role == Qt::CheckStateRole) {
		if (index.column() == 0)
			return checkStates[index.row()] ? Qt::Checked : Qt::Unchecked;
	}
	return QVariant();
}

void DiveImportedModel::changeSelected(QModelIndex clickedIndex)
{
	checkStates[clickedIndex.row()] = !checkStates[clickedIndex.row()];
	dataChanged(index(0, clickedIndex.row()), index(0, clickedIndex.row()), QVector<int>() << Qt::CheckStateRole);
}

void DiveImportedModel::selectAll()
{
	memset(checkStates, true, lastIndex - firstIndex + 1);
	dataChanged(index(0, 0), index(0, lastIndex - firstIndex), QVector<int>() << Qt::CheckStateRole);
}

void DiveImportedModel::selectNone()
{
	memset(checkStates, false, lastIndex - firstIndex + 1);
	dataChanged(index(0, 0), index(0, lastIndex - firstIndex), QVector<int>() << Qt::CheckStateRole);
}

Qt::ItemFlags DiveImportedModel::flags(const QModelIndex &index) const
{
	if (index.column() != 0)
		return QAbstractTableModel::flags(index);
	return QAbstractTableModel::flags(index) | Qt::ItemIsUserCheckable;
}

void DiveImportedModel::clearTable()
{
	beginRemoveRows(QModelIndex(), 0, lastIndex - firstIndex);
	lastIndex = -1;
	firstIndex = 0;
	endRemoveRows();
}

void DiveImportedModel::setImportedDivesIndexes(int first, int last)
{
	Q_ASSERT(last >= first);
	beginRemoveRows(QModelIndex(), 0, lastIndex - firstIndex);
	endRemoveRows();
	beginInsertRows(QModelIndex(), 0, last - first);
	lastIndex = last;
	firstIndex = first;
	delete[] checkStates;
	checkStates = new bool[last - first + 1];
	memset(checkStates, true, last - first + 1);
	endInsertRows();
}
