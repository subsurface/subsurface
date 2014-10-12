#include "downloadfromdivecomputer.h"
#include "../divecomputer.h"
#include "../libdivecomputer.h"
#include "../helpers.h"
#include "../display.h"
#include "../divelist.h"

#include "mainwindow.h"
#include <cstdlib>
#include <QThread>
#include <QDebug>
#include <QStringListModel>
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

DownloadFromDCWidget::DownloadFromDCWidget(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f),
	thread(0),
	downloading(false),
	previousLast(0),
	vendorModel(0),
	productModel(0),
	timer(new QTimer(this)),
	dumpWarningShown(false),
	currentState(INITIAL)
{
	ui.setupUi(this);
	ui.progressBar->hide();
	ui.progressBar->setMinimum(0);
	ui.progressBar->setMaximum(100);

	progress_bar_text = "";

	fill_computer_list();

	ui.chooseDumpFile->setEnabled(ui.dumpToFile->isChecked());
	connect(ui.chooseDumpFile, SIGNAL(clicked()), this, SLOT(pickDumpFile()));
	connect(ui.dumpToFile, SIGNAL(stateChanged(int)), this, SLOT(checkDumpFile(int)));
	ui.chooseLogFile->setEnabled(ui.logToFile->isChecked());
	connect(ui.chooseLogFile, SIGNAL(clicked()), this, SLOT(pickLogFile()));
	connect(ui.logToFile, SIGNAL(stateChanged(int)), this, SLOT(checkLogFile(int)));
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
		ui.cancel->setEnabled(false);
	}

	// user pressed cancel but the application isn't doing anything.
	// means close the window
	else if ((currentState == INITIAL || currentState == CANCELLED || currentState == DONE || currentState == ERROR) && state == CANCELLING) {
		timer->stop();
		reject();
		ui.ok->setText(tr("OK"));
	}

	// the cancelation process is finished
	else if (currentState == CANCELLING && (state == DONE || state == CANCELLED)) {
		timer->stop();
		state = CANCELLED;
		ui.progressBar->setValue(0);
		ui.progressBar->hide();
		markChildrenAsEnabled();
	}

	// DOWNLOAD is finally done, but we don't know if there was an error as libdivecomputer doesn't pass
	// that information on to us
	// so check the progressBar text and if no error was reported, close the dialog and go back to the main window
	// otherwise treat this as if the download was cancelled
	else if (currentState == DOWNLOADING && state == DONE) {
		timer->stop();
		if (QString(progress_bar_text).contains("error", Qt::CaseInsensitive)) {
			updateProgressBar();
			markChildrenAsEnabled();
			progress_bar_text = "";
			ui.ok->setText(tr("Retry"));
		} else {
			ui.progressBar->setValue(100);
			markChildrenAsEnabled();
			ui.ok->setText(tr("OK"));
			accept();
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
		ui.ok->setText(tr("Retry"));
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

void DownloadFromDCWidget::on_cancel_clicked()
{
	updateState(CANCELLING);
}

void DownloadFromDCWidget::on_ok_clicked()
{
	updateState(DOWNLOADING);

	// I don't really think that create/destroy the thread
	// is really necessary.
	if (thread) {
		thread->deleteLater();
	}

	data.devname = strdup(ui.device->currentText().toUtf8().data());
	data.vendor = strdup(ui.vendor->currentText().toUtf8().data());
	data.product = strdup(ui.product->currentText().toUtf8().data());

	data.descriptor = descriptorLookup[ui.vendor->currentText() + ui.product->currentText()];
	data.force_download = ui.forceDownload->isChecked();
	data.create_new_trip = ui.createNewTrip->isChecked();
	data.trip = NULL;
	data.deviceid = data.diveid = 0;
	set_default_dive_computer(data.vendor, data.product);
	set_default_dive_computer_device(data.devname);

	thread = new DownloadThread(this, &data);

	connect(thread, SIGNAL(finished()),
		this, SLOT(onDownloadThreadFinished()), Qt::QueuedConnection);

	MainWindow *w = MainWindow::instance();
	connect(thread, SIGNAL(finished()), w, SLOT(refreshDisplay()));

	// before we start, remember where the dive_table ended
	previousLast = dive_table.nr;

	thread->start();
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

		// I'm not sure if we should really call process_dives even
		// if there's an error
		if (import_thread_cancelled) {
			// walk backwards so we don't keep moving the dives
			// down in the dive_table
			for (int i = dive_table.nr - 1; i >= previousLast; i--)
				delete_single_dive(i);
		} else if (dive_table.nr) {
			int uniqId, idx;
			// remember the last downloaded dive (on most dive computers this will be the chronologically
			// first new dive) and select it again after processing all the dives
			MainWindow::instance()->dive_list()->unselectDives();
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
	} else if (currentState == CANCELLING || currentState == CANCELLED) {
		if (import_thread_cancelled) {
			// walk backwards so we don't keep moving the dives
			// down in the dive_table
			for (int i = dive_table.nr - 1; i >= previousLast; i--)
				delete_single_dive(i);
		}
		updateState(CANCELLED);
	}
}

void DownloadFromDCWidget::markChildrenAsDisabled()
{
	ui.device->setDisabled(true);
	ui.vendor->setDisabled(true);
	ui.product->setDisabled(true);
	ui.forceDownload->setDisabled(true);
	ui.createNewTrip->setDisabled(true);
	ui.preferDownloaded->setDisabled(true);
	ui.ok->setDisabled(true);
	ui.search->setDisabled(true);
	ui.logToFile->setDisabled(true);
	ui.dumpToFile->setDisabled(true);
	ui.chooseLogFile->setDisabled(true);
	ui.chooseDumpFile->setDisabled(true);
}

void DownloadFromDCWidget::markChildrenAsEnabled()
{
	ui.device->setDisabled(false);
	ui.vendor->setDisabled(false);
	ui.product->setDisabled(false);
	ui.forceDownload->setDisabled(false);
	ui.createNewTrip->setDisabled(false);
	ui.preferDownloaded->setDisabled(false);
	ui.ok->setDisabled(false);
	ui.cancel->setDisabled(false);
	ui.search->setDisabled(false);
	ui.logToFile->setDisabled(false);
	ui.dumpToFile->setDisabled(false);
	ui.chooseLogFile->setDisabled(false);
	ui.chooseDumpFile->setDisabled(false);
}

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
	if (!strcmp(data->vendor, "Uemis"))
		errorText = do_uemis_import(data);
	else
		errorText = do_libdivecomputer_import(data);
	if (errorText)
		error = str_error(errorText, data->devname, data->vendor, data->product);
}
