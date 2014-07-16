#include "configuredivecomputerdialog.h"
#include "ui_configuredivecomputerdialog.h"

#include "../divecomputer.h"
#include "../libdivecomputer.h"
#include "../helpers.h"
#include "../display.h"
#include "../divelist.h"
#include "configuredivecomputer.h"
#include <QFileDialog>
#include <QMessageBox>
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
	config(0),
	deviceDetails(0)
{
	ui->setupUi(this);

	deviceDetails = new DeviceDetails(this);
	config = new ConfigureDiveComputer(this);
	connect(config, SIGNAL(error(QString)), this, SLOT(configError(QString)));
	connect(config, SIGNAL(message(QString)), this, SLOT(configMessage(QString)));
	connect(config, SIGNAL(readFinished()), this, SLOT(deviceReadFinished()));
	connect(config, SIGNAL(deviceDetailsChanged(DeviceDetails*)),
		 this, SLOT(deviceDetailsReceived(DeviceDetails*)));
	connect(ui->retrieveDetails, SIGNAL(clicked()), this, SLOT(readSettings()));

	memset(&device_data, 0, sizeof(device_data));
	fill_computer_list();
	if (default_dive_computer_device)
		ui->device->setEditText(default_dive_computer_device);

	ui->DiveComputerList->setCurrentRow(0);
	on_DiveComputerList_currentRowChanged(0);
}

ConfigureDiveComputerDialog::~ConfigureDiveComputerDialog()
{
	delete ui;
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

void ConfigureDiveComputerDialog::fill_computer_list()
{
	dc_iterator_t *iterator = NULL;
	dc_descriptor_t *descriptor = NULL;

	struct mydescriptor *mydescriptor;

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

void ConfigureDiveComputerDialog::populateDeviceDetails()
{
	deviceDetails->setCustomText(ui->customTextLlineEdit->text());
	deviceDetails->setDiveMode(ui->diveModeComboBox->currentIndex());
	deviceDetails->setSaturation(ui->saturationSpinBox->value());
	deviceDetails->setDesaturation(ui->desaturationSpinBox->value());
	deviceDetails->setLastDeco(ui->lastDecoSpinBox->value());
	deviceDetails->setBrightness(ui->brightnessComboBox->currentIndex());
	deviceDetails->setUnits(ui->unitsComboBox->currentIndex());
	deviceDetails->setSamplingRate(ui->samplingRateComboBox->currentIndex());
	deviceDetails->setSalinity(ui->salinitySpinBox->value());
	deviceDetails->setDiveModeColor(ui->diveModeColour->currentIndex());
	deviceDetails->setLanguage(ui->languageComboBox->currentIndex());
	deviceDetails->setDateFormat(ui->dateFormatComboBox->currentIndex());
	deviceDetails->setCompassGain(ui->compassGainComboBox->currentIndex());
	deviceDetails->setSyncTime(ui->dateTimeSyncCheckBox->isChecked());

	//set gas values
	gas gas1;
	gas gas2;
	gas gas3;
	gas gas4;
	gas gas5;

	gas1.oxygen = ui->ostc3GasTable->item(0, 1)->text().toInt();
	gas1.helium = ui->ostc3GasTable->item(0, 2)->text().toInt();
	gas1.type = ui->ostc3GasTable->item(0, 3)->text().toInt();
	gas1.depth = ui->ostc3GasTable->item(0, 4)->text().toInt();

	gas2.oxygen = ui->ostc3GasTable->item(1, 1)->text().toInt();
	gas2.helium = ui->ostc3GasTable->item(1, 2)->text().toInt();
	gas2.type = ui->ostc3GasTable->item(1, 3)->text().toInt();
	gas2.depth = ui->ostc3GasTable->item(1, 4)->text().toInt();

	gas3.oxygen = ui->ostc3GasTable->item(2, 1)->text().toInt();
	gas3.helium = ui->ostc3GasTable->item(2, 2)->text().toInt();
	gas3.type = ui->ostc3GasTable->item(2, 3)->text().toInt();
	gas3.depth = ui->ostc3GasTable->item(2, 4)->text().toInt();

	gas4.oxygen = ui->ostc3GasTable->item(3, 1)->text().toInt();
	gas4.helium = ui->ostc3GasTable->item(3, 2)->text().toInt();
	gas4.type = ui->ostc3GasTable->item(3, 3)->text().toInt();
	gas4.depth = ui->ostc3GasTable->item(3, 4)->text().toInt();

	gas5.oxygen = ui->ostc3GasTable->item(4, 1)->text().toInt();
	gas5.helium = ui->ostc3GasTable->item(4, 2)->text().toInt();
	gas5.type = ui->ostc3GasTable->item(4, 3)->text().toInt();
	gas5.depth = ui->ostc3GasTable->item(4, 4)->text().toInt();

	deviceDetails->setGas1(gas1);
	deviceDetails->setGas2(gas2);
	deviceDetails->setGas3(gas3);
	deviceDetails->setGas4(gas4);
	deviceDetails->setGas5(gas5);

	//set dil values
	gas dil1;
	gas dil2;
	gas dil3;
	gas dil4;
	gas dil5;

	dil1.oxygen = ui->ostc3DilTable->item(0, 1)->text().toInt();
	dil1.helium = ui->ostc3DilTable->item(0, 2)->text().toInt();
	dil1.type = ui->ostc3DilTable->item(0, 3)->text().toInt();
	dil1.depth = ui->ostc3DilTable->item(0, 4)->text().toInt();

	dil2.oxygen = ui->ostc3DilTable->item(1, 1)->text().toInt();
	dil2.helium = ui->ostc3DilTable->item(1, 2)->text().toInt();
	dil2.type = ui->ostc3DilTable->item(1, 3)->text().toInt();
	dil2.depth = ui->ostc3DilTable->item(1, 4)->text().toInt();

	dil3.oxygen = ui->ostc3DilTable->item(2, 1)->text().toInt();
	dil3.helium = ui->ostc3DilTable->item(2, 2)->text().toInt();
	dil3.type = ui->ostc3DilTable->item(2, 3)->text().toInt();
	dil3.depth = ui->ostc3DilTable->item(2, 4)->text().toInt();

	dil4.oxygen = ui->ostc3DilTable->item(3, 1)->text().toInt();
	dil4.helium = ui->ostc3DilTable->item(3, 2)->text().toInt();
	dil4.type = ui->ostc3DilTable->item(3, 3)->text().toInt();
	dil4.depth = ui->ostc3DilTable->item(3, 4)->text().toInt();

	dil5.oxygen = ui->ostc3DilTable->item(4, 1)->text().toInt();
	dil5.helium = ui->ostc3DilTable->item(4, 2)->text().toInt();
	dil5.type = ui->ostc3DilTable->item(4, 3)->text().toInt();
	dil5.depth = ui->ostc3DilTable->item(4, 4)->text().toInt();

	deviceDetails->setDil1(dil1);
	deviceDetails->setDil2(dil2);
	deviceDetails->setDil3(dil3);
	deviceDetails->setDil4(dil4);
	deviceDetails->setDil5(dil5);

	//set set point details
	setpoint sp1;
	setpoint sp2;
	setpoint sp3;
	setpoint sp4;
	setpoint sp5;

	sp1.sp = ui->ostc3SetPointTable->item(0, 1)->text().toInt();
	sp1.depth = ui->ostc3SetPointTable->item(0, 2)->text().toInt();

	sp2.sp = ui->ostc3SetPointTable->item(1, 1)->text().toInt();
	sp2.depth = ui->ostc3SetPointTable->item(1, 2)->text().toInt();

	sp3.sp = ui->ostc3SetPointTable->item(2, 1)->text().toInt();
	sp3.depth = ui->ostc3SetPointTable->item(2, 2)->text().toInt();

	sp4.sp = ui->ostc3SetPointTable->item(3, 1)->text().toInt();
	sp4.depth = ui->ostc3SetPointTable->item(3, 2)->text().toInt();

	sp5.sp = ui->ostc3SetPointTable->item(4, 1)->text().toInt();
	sp5.depth = ui->ostc3SetPointTable->item(4, 2)->text().toInt();
}

void ConfigureDiveComputerDialog::readSettings()
{
	ui->statusLabel->clear();
	ui->errorLabel->clear();

	getDeviceData();
	config->readSettings(&device_data);
}

void ConfigureDiveComputerDialog::configMessage(QString msg)
{
	ui->statusLabel->setText(msg);
}

void ConfigureDiveComputerDialog::configError(QString err)
{
	ui->errorLabel->setText(err);
}

void ConfigureDiveComputerDialog::getDeviceData()
{
	device_data.devname = strdup(ui->device->currentText().toUtf8().data());
	device_data.vendor = strdup(selected_vendor.toUtf8().data());
	device_data.product = strdup(selected_product.toUtf8().data());

	device_data.descriptor = descriptorLookup[selected_vendor + selected_product];
	device_data.deviceid = device_data.diveid = 0;

	set_default_dive_computer_device(device_data.devname);
}

void ConfigureDiveComputerDialog::on_cancel_clicked()
{
	this->close();
}

void ConfigureDiveComputerDialog::deviceReadFinished()
{
	ui->statusLabel->setText(tr("Dive computer details read successfully."));
}

void ConfigureDiveComputerDialog::on_saveSettingsPushButton_clicked()
{
	populateDeviceDetails();
	getDeviceData();
	config->saveDeviceDetails(deviceDetails, &device_data);
}

void ConfigureDiveComputerDialog::deviceDetailsReceived(DeviceDetails *newDeviceDetails)
{
	deviceDetails = newDeviceDetails;
	reloadValues();
}

void ConfigureDiveComputerDialog::reloadValues()
{
	ui->serialNoLineEdit->setText(deviceDetails->serialNo());
	ui->firmwareVersionLineEdit->setText(deviceDetails->firmwareVersion());
	ui->customTextLlineEdit->setText(deviceDetails->customText());
	ui->diveModeComboBox->setCurrentIndex(deviceDetails->diveMode());
	ui->saturationSpinBox->setValue(deviceDetails->saturation());
	ui->desaturationSpinBox->setValue(deviceDetails->desaturation());
	ui->lastDecoSpinBox->setValue(deviceDetails->lastDeco());
	ui->brightnessComboBox->setCurrentIndex(deviceDetails->brightness());
	ui->unitsComboBox->setCurrentIndex(deviceDetails->units());
	ui->samplingRateComboBox->setCurrentIndex(deviceDetails->samplingRate());
	ui->salinitySpinBox->setValue(deviceDetails->salinity());
	ui->diveModeColour->setCurrentIndex(deviceDetails->diveModeColor());
	ui->languageComboBox->setCurrentIndex(deviceDetails->language());
	ui->dateFormatComboBox->setCurrentIndex(deviceDetails->dateFormat());
	ui->compassGainComboBox->setCurrentIndex(deviceDetails->compassGain());

	//load gas 1 values
	ui->ostc3GasTable->setItem(0,1, new QTableWidgetItem(QString::number(deviceDetails->gas1().oxygen)));
	ui->ostc3GasTable->setItem(0,2, new QTableWidgetItem(QString::number(deviceDetails->gas1().helium)));
	ui->ostc3GasTable->setItem(0,3, new QTableWidgetItem(QString::number(deviceDetails->gas1().type)));
	ui->ostc3GasTable->setItem(0,4, new QTableWidgetItem(QString::number(deviceDetails->gas1().depth)));

	//load gas 2 values
	ui->ostc3GasTable->setItem(1,1, new QTableWidgetItem(QString::number(deviceDetails->gas2().oxygen)));
	ui->ostc3GasTable->setItem(1,2, new QTableWidgetItem(QString::number(deviceDetails->gas2().helium)));
	ui->ostc3GasTable->setItem(1,3, new QTableWidgetItem(QString::number(deviceDetails->gas2().type)));
	ui->ostc3GasTable->setItem(1,4, new QTableWidgetItem(QString::number(deviceDetails->gas2().depth)));

	//load gas 3 values
	ui->ostc3GasTable->setItem(2,1, new QTableWidgetItem(QString::number(deviceDetails->gas3().oxygen)));
	ui->ostc3GasTable->setItem(2,2, new QTableWidgetItem(QString::number(deviceDetails->gas3().helium)));
	ui->ostc3GasTable->setItem(2,3, new QTableWidgetItem(QString::number(deviceDetails->gas3().type)));
	ui->ostc3GasTable->setItem(2,4, new QTableWidgetItem(QString::number(deviceDetails->gas3().depth)));

	//load gas 4 values
	ui->ostc3GasTable->setItem(3,1, new QTableWidgetItem(QString::number(deviceDetails->gas4().oxygen)));
	ui->ostc3GasTable->setItem(3,2, new QTableWidgetItem(QString::number(deviceDetails->gas4().helium)));
	ui->ostc3GasTable->setItem(3,3, new QTableWidgetItem(QString::number(deviceDetails->gas4().type)));
	ui->ostc3GasTable->setItem(3,4, new QTableWidgetItem(QString::number(deviceDetails->gas4().depth)));

	//load gas 5 values
	ui->ostc3GasTable->setItem(4,1, new QTableWidgetItem(QString::number(deviceDetails->gas5().oxygen)));
	ui->ostc3GasTable->setItem(4,2, new QTableWidgetItem(QString::number(deviceDetails->gas5().helium)));
	ui->ostc3GasTable->setItem(4,3, new QTableWidgetItem(QString::number(deviceDetails->gas5().type)));
	ui->ostc3GasTable->setItem(4,4, new QTableWidgetItem(QString::number(deviceDetails->gas5().depth)));

	//load dil 1 values
	ui->ostc3DilTable->setItem(0,1, new QTableWidgetItem(QString::number(deviceDetails->dil1().oxygen)));
	ui->ostc3DilTable->setItem(0,2, new QTableWidgetItem(QString::number(deviceDetails->dil1().helium)));
	ui->ostc3DilTable->setItem(0,3, new QTableWidgetItem(QString::number(deviceDetails->dil1().type)));
	ui->ostc3DilTable->setItem(0,4, new QTableWidgetItem(QString::number(deviceDetails->dil1().depth)));

	//load dil 2 values
	ui->ostc3DilTable->setItem(1,1, new QTableWidgetItem(QString::number(deviceDetails->dil2().oxygen)));
	ui->ostc3DilTable->setItem(1,2, new QTableWidgetItem(QString::number(deviceDetails->dil2().helium)));
	ui->ostc3DilTable->setItem(1,3, new QTableWidgetItem(QString::number(deviceDetails->dil2().type)));
	ui->ostc3DilTable->setItem(1,4, new QTableWidgetItem(QString::number(deviceDetails->dil2().depth)));

	//load dil 3 values
	ui->ostc3DilTable->setItem(2,1, new QTableWidgetItem(QString::number(deviceDetails->dil3().oxygen)));
	ui->ostc3DilTable->setItem(2,2, new QTableWidgetItem(QString::number(deviceDetails->dil3().helium)));
	ui->ostc3DilTable->setItem(2,3, new QTableWidgetItem(QString::number(deviceDetails->dil3().type)));
	ui->ostc3DilTable->setItem(2,4, new QTableWidgetItem(QString::number(deviceDetails->dil3().depth)));

	//load dil 4 values
	ui->ostc3DilTable->setItem(3,1, new QTableWidgetItem(QString::number(deviceDetails->dil4().oxygen)));
	ui->ostc3DilTable->setItem(3,2, new QTableWidgetItem(QString::number(deviceDetails->dil4().helium)));
	ui->ostc3DilTable->setItem(3,3, new QTableWidgetItem(QString::number(deviceDetails->dil4().type)));
	ui->ostc3DilTable->setItem(3,4, new QTableWidgetItem(QString::number(deviceDetails->dil4().depth)));

	//load dil 5 values
	ui->ostc3DilTable->setItem(4,1, new QTableWidgetItem(QString::number(deviceDetails->dil5().oxygen)));
	ui->ostc3DilTable->setItem(4,2, new QTableWidgetItem(QString::number(deviceDetails->dil5().helium)));
	ui->ostc3DilTable->setItem(4,3, new QTableWidgetItem(QString::number(deviceDetails->dil5().type)));
	ui->ostc3DilTable->setItem(4,4, new QTableWidgetItem(QString::number(deviceDetails->dil5().depth)));

	//load set point 1 values
	ui->ostc3SetPointTable->setItem(0, 1, new QTableWidgetItem(QString::number(deviceDetails->sp1().sp)));
	ui->ostc3SetPointTable->setItem(0, 2, new QTableWidgetItem(QString::number(deviceDetails->sp1().depth)));

	//load set point 2 values
	ui->ostc3SetPointTable->setItem(1, 1, new QTableWidgetItem(QString::number(deviceDetails->sp2().sp)));
	ui->ostc3SetPointTable->setItem(1, 2, new QTableWidgetItem(QString::number(deviceDetails->sp2().depth)));

	//load set point 3 values
	ui->ostc3SetPointTable->setItem(2, 1, new QTableWidgetItem(QString::number(deviceDetails->sp3().sp)));
	ui->ostc3SetPointTable->setItem(2, 2, new QTableWidgetItem(QString::number(deviceDetails->sp3().depth)));

	//load set point 4 values
	ui->ostc3SetPointTable->setItem(3, 1, new QTableWidgetItem(QString::number(deviceDetails->sp4().sp)));
	ui->ostc3SetPointTable->setItem(3, 2, new QTableWidgetItem(QString::number(deviceDetails->sp4().depth)));

	//load set point 5 values
	ui->ostc3SetPointTable->setItem(4, 1, new QTableWidgetItem(QString::number(deviceDetails->sp5().sp)));
	ui->ostc3SetPointTable->setItem(4, 2, new QTableWidgetItem(QString::number(deviceDetails->sp5().depth)));
}


void ConfigureDiveComputerDialog::on_backupButton_clicked()
{
	QString filename = existing_filename ?: prefs.default_filename;
	QFileInfo fi(filename);
	filename = fi.absolutePath().append(QDir::separator()).append("Backup.xml");
	QString backupPath = QFileDialog::getSaveFileName(this, tr("Backup Dive Computer Settings"),
							  filename, tr("Backup files (*.xml)")
							  );
	if (!backupPath.isEmpty()) {
		populateDeviceDetails();
		getDeviceData();
		QString errorText = "";
		if (!config->saveXMLBackup(backupPath, deviceDetails, &device_data, errorText)) {
			QMessageBox::critical(this, tr("XML Backup Error"),
					      tr("An error occurred while saving the backup file.\n%1")
					      .arg(errorText)
					      );
		} else {
			QMessageBox::information(this, tr("Backup succeeded"),
						 tr("Your settings have been saved to: %1")
						 .arg(filename)
						 );
		}
	}
}

void ConfigureDiveComputerDialog::on_restoreBackupButton_clicked()
{
	QString filename = existing_filename ?: prefs.default_filename;
	QFileInfo fi(filename);
	filename = fi.absolutePath().append(QDir::separator()).append("Backup.xml");
	QString restorePath = QFileDialog::getOpenFileName(this, tr("Restore Dive Computer Settings"),
							   filename, tr("Backup files (*.xml)")
							   );
	if (!restorePath.isEmpty()) {
		QString errorText = "";
		if (!config->restoreXMLBackup(restorePath, deviceDetails, errorText)) {
			QMessageBox::critical(this, tr("XML Restore Error"),
					      tr("An error occurred while restoring the backup file.\n%1")
					      .arg(errorText)
					      );
		} else {
			reloadValues();
			//getDeviceData();
			//config->saveDeviceDetails(deviceDetails, &device_data);
			QMessageBox::information(this, tr("Restore succeeded"),
						 tr("Your settings have been restored successfully.")
						 );
		}
	}
}

void ConfigureDiveComputerDialog::on_updateFirmwareButton_clicked()
{
	QString filename = existing_filename ?: prefs.default_filename;
	QFileInfo fi(filename);
	filename = fi.absolutePath();
	QString firmwarePath = QFileDialog::getOpenFileName(this, tr("Select firmware file"),
							  filename, tr("All files (*.*)")
							  );
	if (!firmwarePath.isEmpty()) {
		getDeviceData();
		QString errText;
		config->startFirmwareUpdate(firmwarePath, &device_data, errText);
	}
}

void ConfigureDiveComputerDialog::on_DiveComputerList_currentRowChanged(int currentRow)
{
	switch (currentRow) {
	case 0:
		selected_vendor = "Heinrichs Weikamp";
		selected_product = "OSTC 3";
		break;
	}

	int dcType = DC_TYPE_SERIAL;


	if (selected_vendor == QString("Uemis"))
		dcType = DC_TYPE_UEMIS;
	fill_device_list(dcType);
}
