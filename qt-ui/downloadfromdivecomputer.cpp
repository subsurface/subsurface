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
	return dialog;
}

DownloadFromDCWidget::DownloadFromDCWidget(QWidget* parent, Qt::WindowFlags f) :
	QDialog(parent, f), ui(new Ui::DownloadFromDiveComputer), thread(0), downloading(false)
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
}

void DownloadFromDCWidget::runDialog()
{
	ui->progressBar->hide();
	show();
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
	import_thread_cancelled = true;
	if (thread) {
		thread->wait();
		thread->deleteLater();
		thread = 0;
	}
	close();
}

void DownloadFromDCWidget::on_ok_clicked()
{
	if (downloading)
		return;

	ui->progressBar->setValue(0);
	ui->progressBar->show();

	if (thread) {
		thread->deleteLater();
	}

	data.devname = strdup(ui->device->text().toUtf8().data());
	data.vendor = strdup(ui->vendor->currentText().toUtf8().data());
	data.product = strdup(ui->product->currentText().toUtf8().data());
	data.descriptor = descriptorLookup[ui->vendor->currentText() + ui->product->currentText()];
	data.force_download = ui->forceDownload->isChecked();
	set_default_dive_computer(data.vendor, data.product);
	set_default_dive_computer_device(data.devname);

	thread = new InterfaceThread(this, &data);
	connect(thread, SIGNAL(updateInterface(int)),
			ui->progressBar, SLOT(setValue(int)), Qt::QueuedConnection); // Qt::QueuedConnection == threadsafe.

	connect(thread, SIGNAL(finished()), this, SLOT(close()));

	thread->start();
	downloading = true;
}

bool DownloadFromDCWidget::preferDownloaded()
{
	return ui->preferDownloaded->isChecked();
}

DownloadThread::DownloadThread(device_data_t* data): data(data)
{
}

void DownloadThread::run()
{
	DownloadFromDCWidget *dfdcw = DownloadFromDCWidget::instance();
	do_libdivecomputer_import(data);
	process_dives(TRUE, dfdcw->preferDownloaded());
}

InterfaceThread::InterfaceThread(QObject* parent, device_data_t* data): QThread(parent), data(data)
{
}

void InterfaceThread::run()
{
	DownloadThread *download = new DownloadThread(data);
	MainWindow *w = mainWindow();
	connect(download, SIGNAL(finished()), w, SLOT(refreshDisplay()));
	download->start();
	while (download->isRunning()) {
		msleep(200);
		updateInterface(progress_bar_fraction *100);
	}
	updateInterface(100);
}
