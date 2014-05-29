#include "configuredivecomputerdialog.h"
#include "ui_configuredivecomputerdialog.h"

#include "../divecomputer.h"
#include "../libdivecomputer.h"
#include "../helpers.h"
#include "../display.h"
#include "../divelist.h"
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

ConfigureDiveComputerDialog::ConfigureDiveComputerDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ConfigureDiveComputerDialog),
	vendorModel(0),
	productModel(0)
{
	ui->setupUi(this);

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
		ui->device->setEditText(default_dive_computer_device);
}

ConfigureDiveComputerDialog::~ConfigureDiveComputerDialog()
{
	delete ui;
}

void ConfigureDiveComputerDialog::fill_computer_list()
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

static void fillDeviceList(const char *name, void *data)
{
	QComboBox *comboBox = (QComboBox *)data;
	comboBox->addItem(name);
}

void ConfigureDiveComputerDialog::fill_device_list(int dc_type)
{
	int deviceIndex;
	ui->device->clear();
	deviceIndex = enumerate_devices(fillDeviceList, ui->device, dc_type);
	if (deviceIndex >= 0)
		ui->device->setCurrentIndex(deviceIndex);
}

void ConfigureDiveComputerDialog::on_vendor_currentIndexChanged(const QString &vendor)
{
	int dcType = DC_TYPE_SERIAL;
	QAbstractItemModel *currentModel = ui->product->model();
	if (!currentModel)
		return;

	productModel = new QStringListModel(productList[vendor]);
	ui->product->setModel(productModel);

	if (vendor == QString("Uemis"))
		dcType = DC_TYPE_UEMIS;
	fill_device_list(dcType);
}

void ConfigureDiveComputerDialog::on_product_currentIndexChanged(const QString &product)
{
	dc_descriptor_t *descriptor = NULL;
	descriptor = descriptorLookup[ui->vendor->currentText() + product];

	// call dc_descriptor_get_transport to see if the dc_transport_t is DC_TRANSPORT_SERIAL
	if (dc_descriptor_get_transport(descriptor) == DC_TRANSPORT_SERIAL) {
		// if the dc_transport_t is DC_TRANSPORT_SERIAL, then enable the device node box.
		ui->device->setEnabled(true);
	} else {
		// otherwise disable the device node box
		ui->device->setEnabled(false);
	}
}
