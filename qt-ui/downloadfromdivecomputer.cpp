#include "downloadfromdivecomputer.h"
#include "ui_downloadfromdivecomputer.h"

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

namespace DownloadFromDcGlobal{
	const char *err_string;
};

DownloadFromDCWidget *DownloadFromDCWidget::instance()
{
	static DownloadFromDCWidget *dialog = new DownloadFromDCWidget();
	dialog->setAttribute(Qt::WA_QuitOnClose, false);
	return dialog;
}

DownloadFromDCWidget::DownloadFromDCWidget(QWidget* parent, Qt::WindowFlags f) :
	QDialog(parent, f), ui(new Ui::DownloadFromDiveComputer), thread(0), timer(new QTimer(this)),
	currentState(INITIAL)
{
	ui->setupUi(this);
	ui->progressBar->hide();
	ui->progressBar->setMinimum(0);
	ui->progressBar->setMaximum(100);
	fill_computer_list();

	vendorModel = new QStringListModel(vendorList);
	ui->vendor->setModel(vendorModel);
	if (default_dive_computer_vendor) {
		ui->vendor->setCurrentIndex(ui->vendor->findText(default_dive_computer_vendor));
		productModel = new QStringListModel(productList[default_dive_computer_vendor]);
		ui->product->setModel(productModel);
		if (default_dive_computer_product)
			ui->product->setCurrentIndex(ui->product->findText(default_dive_computer_product));
	}
	if (default_dive_computer_device)
		ui->device->setText(default_dive_computer_device);

	timer->setInterval(200);
	connect(timer, SIGNAL(timeout()), this, SLOT(updateProgressBar()));

	updateState(INITIAL);
}

void DownloadFromDCWidget::runDialog()
{
	updateState(INITIAL);

	exec();
}

void DownloadFromDCWidget::updateProgressBar()
{
	ui->progressBar->setValue(progress_bar_fraction *100);
}

void DownloadFromDCWidget::updateState(states state)
{
	if (state == currentState)
		return;

	if (state == INITIAL) {
		ui->progressBar->hide();
		markChildrenAsEnabled();
		timer->stop();
	}

	// tries to cancel an on going download
	else if (currentState == DOWNLOADING && state == CANCELLING) {
		import_thread_cancelled = true;
		ui->cancel->setEnabled(false);
	}

	// user pressed cancel but the application isn't doing anything.
	// means close the window
	else if ((currentState == INITIAL || currentState == CANCELLED || currentState == DONE)
		&& state == CANCELLING) {
		timer->stop();
		reject();
	}

	// A cancel is finished
	else if (currentState == CANCELLING && (state == DONE || state == CANCELLED)) {
		timer->stop();
		state = CANCELLED;
		ui->progressBar->setValue(0);
		ui->progressbar->hide();
		markChildrenAsEnabled();
	}

	// DOWNLOAD is finally done, close the dialog and go back to the main window
	else if (currentState == DOWNLOADING && state == DONE) {
		timer->stop();
		ui->progressBar->setValue(100);
		markChildrenAsEnabled();
		accept();
	}

	// DOWNLOAD is started.
	else if (state == DOWNLOADING) {
		timer->start();
		ui->progressBar->setValue(0);
		ui->progressBar->show();
		markChildrenAsDisabled();
	}

	// properly updating the widget state
	currentState = state;
}

void DownloadFromDCWidget::on_vendor_currentIndexChanged(const QString& vendor)
{
	QAbstractItemModel *currentModel = ui->product->model();
	if (!currentModel)
		return;

	productModel = new QStringListModel(productList[vendor]);
	ui->product->setModel(productModel);

	// Memleak - but deleting gives me a crash.
	//currentModel->deleteLater();
}

void DownloadFromDCWidget::fill_computer_list()
{
	dc_iterator_t *iterator = NULL;
	dc_descriptor_t *descriptor = NULL;
	struct mydescriptor *mydescriptor;

	QStringList computer;
	dc_descriptor_iterator(&iterator);
	while (dc_iterator_next (iterator, &descriptor) == DC_STATUS_SUCCESS) {
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

	mydescriptor = (struct mydescriptor*) malloc(sizeof(struct mydescriptor));
	mydescriptor->vendor = "Uemis";
	mydescriptor->product = "Zurich";
	mydescriptor->type = DC_FAMILY_NULL;
	mydescriptor->model = 0;

	if (!vendorList.contains("Uemis"))
		vendorList.append("Uemis");

	if (!productList["Uemis"].contains("Zurich"))
		productList["Uemis"].push_back("Zurich");

	descriptorLookup[QString("UemisZurich")] = (dc_descriptor_t *)mydescriptor;
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

	data.devname = strdup(ui->device->text().toUtf8().data());
	data.vendor = strdup(ui->vendor->currentText().toUtf8().data());
	data.product = strdup(ui->product->currentText().toUtf8().data());

	data.descriptor = descriptorLookup[ui->vendor->currentText() + ui->product->currentText()];
	data.force_download = ui->forceDownload->isChecked();
	data.deviceid = data.diveid = 0;
	set_default_dive_computer(data.vendor, data.product);
	set_default_dive_computer_device(data.devname);

	thread = new DownloadThread(this, &data);

	connect(thread, SIGNAL(finished()),
			this, SLOT(onDownloadThreadFinished()), Qt::QueuedConnection);

	MainWindow *w = mainWindow();
	connect(thread, SIGNAL(finished()), w, SLOT(refreshDisplay()));

	thread->start();
}

bool DownloadFromDCWidget::preferDownloaded()
{
	return ui->preferDownloaded->isChecked();
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
	if (currentState == DOWNLOADING)
		updateState(DONE);
	else
		updateState(CANCELLED);
}

void DownloadFromDCWidget::markChildrenAsDisabled()
{
	ui->device->setDisabled(true);
	ui->vendor->setDisabled(true);
	ui->product->setDisabled(true);
	ui->forceDownload->setDisabled(true);
	ui->preferDownloaded->setDisabled(true);
	ui->ok->setDisabled(true);
	ui->search->setDisabled(true);
}

void DownloadFromDCWidget::markChildrenAsEnabled()
{
	ui->device->setDisabled(false);
	ui->vendor->setDisabled(false);
	ui->product->setDisabled(false);
	ui->forceDownload->setDisabled(false);
	ui->preferDownloaded->setDisabled(false);
	ui->ok->setDisabled(false);
	ui->cancel->setDisabled(false);
	ui->search->setDisabled(false);
}

DownloadThread::DownloadThread(QObject* parent, device_data_t* data): QThread(parent),
	data(data)
{
}

void DownloadThread::run()
{
	DownloadFromDCWidget *dfdcw = DownloadFromDCWidget::instance();

	if (!strcmp(data->vendor, "Uemis"))
		do_uemis_import(data->devname, data->force_download);
	else
		// TODO: implement error handling
		do_libdivecomputer_import(data);

	// I'm not sure if we should really process_dives even
	// if there's an error or a cancelation
	process_dives(TRUE, dfdcw->preferDownloaded());
}
