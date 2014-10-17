#include "configuredivecomputerdialog.h"

#include "../divecomputer.h"
#include "../libdivecomputer.h"
#include "../helpers.h"
#include "../display.h"
#include "../divelist.h"
#include "configuredivecomputer.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
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
	config(0),
	deviceDetails(0)
{
	ui.setupUi(this);

	deviceDetails = new DeviceDetails(this);
	config = new ConfigureDiveComputer(this);
	connect(config, SIGNAL(error(QString)), this, SLOT(configError(QString)));
	connect(config, SIGNAL(message(QString)), this, SLOT(configMessage(QString)));
	connect(config, SIGNAL(readFinished()), this, SLOT(deviceReadFinished()));
	connect(config, SIGNAL(deviceDetailsChanged(DeviceDetails*)),
		 this, SLOT(deviceDetailsReceived(DeviceDetails*)));
	connect(ui.retrieveDetails, SIGNAL(clicked()), this, SLOT(readSettings()));

	memset(&device_data, 0, sizeof(device_data));
	fill_computer_list();
	if (default_dive_computer_device)
		ui.device->setEditText(default_dive_computer_device);

	ui.DiveComputerList->setCurrentRow(0);
	on_DiveComputerList_currentRowChanged(0);

	QSettings settings;
	settings.beginGroup("ConfigureDiveComputerDialog");
	settings.beginGroup("ostc3GasTable");
	for (int i = 0; i < ui.ostc3GasTable->columnCount(); i++) {
        QVariant width = settings.value(QString("colwidth%1").arg(i));
		if (width.isValid())
			ui.ostc3GasTable->setColumnWidth(i, width.toInt());
	}
	settings.endGroup();
	settings.beginGroup("ostc3DilTable");
	for (int i = 0; i < ui.ostc3DilTable->columnCount(); i++) {
        QVariant width = settings.value(QString("colwidth%1").arg(i));
		if (width.isValid())
			ui.ostc3DilTable->setColumnWidth(i, width.toInt());
	}
	settings.endGroup();
	settings.beginGroup("ostc3SetPointTable");
	for (int i = 0; i < ui.ostc3SetPointTable->columnCount(); i++) {
        QVariant width = settings.value(QString("colwidth%1").arg(i));
		if (width.isValid())
			ui.ostc3SetPointTable->setColumnWidth(i, width.toInt());
	}
	settings.endGroup();
	settings.endGroup();
}

ConfigureDiveComputerDialog::~ConfigureDiveComputerDialog()
{
	QSettings settings;
	settings.beginGroup("ConfigureDiveComputerDialog");
	settings.beginGroup("ostc3GasTable");
	for (int i = 0; i < ui.ostc3GasTable->columnCount(); i++)
		settings.setValue(QString("colwidth%1").arg(i), ui.ostc3GasTable->columnWidth(i));
	settings.endGroup();
	settings.beginGroup("ostc3DilTable");
	for (int i = 0; i < ui.ostc3DilTable->columnCount(); i++)
		settings.setValue(QString("colwidth%1").arg(i), ui.ostc3DilTable->columnWidth(i));
	settings.endGroup();
	settings.beginGroup("ostc3SetPointTable");
	for (int i = 0; i < ui.ostc3SetPointTable->columnCount(); i++)
		settings.setValue(QString("colwidth%1").arg(i), ui.ostc3SetPointTable->columnWidth(i));
	settings.endGroup();
	settings.endGroup();
}


static void fillDeviceList(const char *name, void *data)
{
	QComboBox *comboBox = (QComboBox *)data;
	comboBox->addItem(name);
}

void ConfigureDiveComputerDialog::fill_device_list(int dc_type)
{
	int deviceIndex;
	ui.device->clear();
	deviceIndex = enumerate_devices(fillDeviceList, ui.device, dc_type);
	if (deviceIndex >= 0)
		ui.device->setCurrentIndex(deviceIndex);
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
	switch(ui.dcStackedWidget->currentIndex()) {
	case 0:
		populateDeviceDetailsOSTC3();
		break;
	case 1:
		populateDeviceDetailsSuuntoVyper();
		break;
	}
}

#define GET_INT_FROM(_field, _default) ((_field) != NULL) ? (_field)->text().toInt() : (_default)

void ConfigureDiveComputerDialog::populateDeviceDetailsOSTC3()
{
	deviceDetails->setCustomText(ui.customTextLlineEdit->text());
	deviceDetails->setDiveMode(ui.diveModeComboBox->currentIndex());
	deviceDetails->setSaturation(ui.saturationSpinBox->value());
	deviceDetails->setDesaturation(ui.desaturationSpinBox->value());
	deviceDetails->setLastDeco(ui.lastDecoSpinBox->value());
	deviceDetails->setBrightness(ui.brightnessComboBox->currentIndex());
	deviceDetails->setUnits(ui.unitsComboBox->currentIndex());
	deviceDetails->setSamplingRate(ui.samplingRateComboBox->currentIndex());
	deviceDetails->setSalinity(ui.salinitySpinBox->value());
	deviceDetails->setDiveModeColor(ui.diveModeColour->currentIndex());
	deviceDetails->setLanguage(ui.languageComboBox->currentIndex());
	deviceDetails->setDateFormat(ui.dateFormatComboBox->currentIndex());
	deviceDetails->setCompassGain(ui.compassGainComboBox->currentIndex());
	deviceDetails->setSyncTime(ui.dateTimeSyncCheckBox->isChecked());
	deviceDetails->setSafetyStop(ui.safetyStopCheckBox->isChecked());
	deviceDetails->setGfHigh(ui.gfHighSpinBox->value());
	deviceDetails->setGfLow(ui.gfLowSpinBox->value());
	deviceDetails->setPressureSensorOffset(ui.pressureSensorOffsetSpinBox->value());
	deviceDetails->setPpO2Min(ui.ppO2MinSpinBox->value());
	deviceDetails->setPpO2Max(ui.ppO2MaxSpinBox->value());
	deviceDetails->setFutureTTS(ui.futureTTSSpinBox->value());
	deviceDetails->setCcrMode(ui.ccrModeComboBox->currentIndex());
	deviceDetails->setDecoType(ui.decoTypeComboBox->currentIndex());
	deviceDetails->setAGFSelectable(ui.aGFSelectableCheckBox->isChecked());
	deviceDetails->setAGFHigh(ui.aGFHighSpinBox->value());
	deviceDetails->setAGFLow(ui.aGFLowSpinBox->value());
	deviceDetails->setCalibrationGas(ui.calibrationGasSpinBox->value());
	deviceDetails->setFlipScreen(ui.flipScreenCheckBox->isChecked());
	deviceDetails->setSetPointFallback(ui.setPointFallbackCheckBox->isChecked());

	//set gas values
	gas gas1;
	gas gas2;
	gas gas3;
	gas gas4;
	gas gas5;

	gas1.oxygen = GET_INT_FROM(ui.ostc3GasTable->item(0, 1), 21);
	gas1.helium = GET_INT_FROM(ui.ostc3GasTable->item(0, 2), 0);
	gas1.type = GET_INT_FROM(ui.ostc3GasTable->item(0, 3), 0);
	gas1.depth = GET_INT_FROM(ui.ostc3GasTable->item(0, 4), 0);

	gas2.oxygen = GET_INT_FROM(ui.ostc3GasTable->item(1, 1), 21);
	gas2.helium = GET_INT_FROM(ui.ostc3GasTable->item(1, 2), 0);
	gas2.type = GET_INT_FROM(ui.ostc3GasTable->item(1, 3), 0);
	gas2.depth = GET_INT_FROM(ui.ostc3GasTable->item(1, 4), 0);

	gas3.oxygen = GET_INT_FROM(ui.ostc3GasTable->item(2, 1), 21);
	gas3.helium = GET_INT_FROM(ui.ostc3GasTable->item(2, 2), 0);
	gas3.type = GET_INT_FROM(ui.ostc3GasTable->item(2, 3), 0);
	gas3.depth = GET_INT_FROM(ui.ostc3GasTable->item(2, 4), 0);

	gas4.oxygen = GET_INT_FROM(ui.ostc3GasTable->item(3, 1), 21);
	gas4.helium = GET_INT_FROM(ui.ostc3GasTable->item(3, 2), 0);
	gas4.type = GET_INT_FROM(ui.ostc3GasTable->item(3, 3), 0);
	gas4.depth = GET_INT_FROM(ui.ostc3GasTable->item(3, 4), 0);

	gas5.oxygen = GET_INT_FROM(ui.ostc3GasTable->item(4, 1), 21);
	gas5.helium = GET_INT_FROM(ui.ostc3GasTable->item(4, 2), 0);
	gas5.type = GET_INT_FROM(ui.ostc3GasTable->item(4, 3), 0);
	gas5.depth = GET_INT_FROM(ui.ostc3GasTable->item(4, 4), 0);

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

	dil1.oxygen = GET_INT_FROM(ui.ostc3DilTable->item(0, 1), 21);
	dil1.helium = GET_INT_FROM(ui.ostc3DilTable->item(0, 2), 0);
	dil1.type = GET_INT_FROM(ui.ostc3DilTable->item(0, 3), 0);
	dil1.depth = GET_INT_FROM(ui.ostc3DilTable->item(0, 4), 0);

	dil2.oxygen = GET_INT_FROM(ui.ostc3DilTable->item(1, 1), 21);
	dil2.helium = GET_INT_FROM(ui.ostc3DilTable->item(1, 2), 0);
	dil2.type = GET_INT_FROM(ui.ostc3DilTable->item(1, 3), 0);
	dil2.depth = GET_INT_FROM(ui.ostc3DilTable->item(1, 4),0);

	dil3.oxygen = GET_INT_FROM(ui.ostc3DilTable->item(2, 1), 21);
	dil3.helium = GET_INT_FROM(ui.ostc3DilTable->item(2, 2), 0);
	dil3.type = GET_INT_FROM(ui.ostc3DilTable->item(2, 3), 0);
	dil3.depth = GET_INT_FROM(ui.ostc3DilTable->item(2, 4), 0);

	dil4.oxygen = GET_INT_FROM(ui.ostc3DilTable->item(3, 1), 21);
	dil4.helium = GET_INT_FROM(ui.ostc3DilTable->item(3, 2), 0);
	dil4.type = GET_INT_FROM(ui.ostc3DilTable->item(3, 3), 0);
	dil4.depth = GET_INT_FROM(ui.ostc3DilTable->item(3, 4), 0);

	dil5.oxygen = GET_INT_FROM(ui.ostc3DilTable->item(4, 1), 21);
	dil5.helium = GET_INT_FROM(ui.ostc3DilTable->item(4, 2), 0);
	dil5.type = GET_INT_FROM(ui.ostc3DilTable->item(4, 3), 0);
	dil5.depth = GET_INT_FROM(ui.ostc3DilTable->item(4, 4), 0);

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

	sp1.sp = GET_INT_FROM(ui.ostc3SetPointTable->item(0, 1), 70);
	sp1.depth = GET_INT_FROM(ui.ostc3SetPointTable->item(0, 2), 0);

	sp2.sp = GET_INT_FROM(ui.ostc3SetPointTable->item(1, 1), 90);
	sp2.depth = GET_INT_FROM(ui.ostc3SetPointTable->item(1, 2), 20);

	sp3.sp = GET_INT_FROM(ui.ostc3SetPointTable->item(2, 1), 100);
	sp3.depth = GET_INT_FROM(ui.ostc3SetPointTable->item(2, 2), 33);

	sp4.sp = GET_INT_FROM(ui.ostc3SetPointTable->item(3, 1), 120);
	sp4.depth = GET_INT_FROM(ui.ostc3SetPointTable->item(3, 2), 50);

	sp5.sp = GET_INT_FROM(ui.ostc3SetPointTable->item(4, 1), 140);
	sp5.depth = GET_INT_FROM(ui.ostc3SetPointTable->item(4, 2), 70);

	deviceDetails->setSp1(sp1);
	deviceDetails->setSp2(sp2);
	deviceDetails->setSp3(sp3);
	deviceDetails->setSp4(sp4);
	deviceDetails->setSp5(sp5);
}

void ConfigureDiveComputerDialog::populateDeviceDetailsSuuntoVyper()
{
	deviceDetails->setCustomText(ui.customTextLlineEdit_1->text());
	deviceDetails->setSamplingRate(ui.samplingRateComboBox_1->currentIndex() == 3 ? 60 : (ui.samplingRateComboBox_1->currentIndex() + 1) * 10);
	deviceDetails->setAltitude(ui.altitudeRangeComboBox->currentIndex());
	deviceDetails->setPersonalSafety(ui.personalSafetyComboBox->currentIndex());
	deviceDetails->setTimeFormat(ui.timeFormatComboBox->currentIndex());
	deviceDetails->setUnits(ui.unitsComboBox_1->currentIndex());
	deviceDetails->setDiveMode(ui.diveModeComboBox_1->currentIndex());
	deviceDetails->setLightEnabled(ui.lightCheckBox->isChecked());
	deviceDetails->setLight(ui.lightSpinBox->value());
	deviceDetails->setAlarmDepthEnabled(ui.alarmDepthCheckBox->isChecked());
	deviceDetails->setAlarmDepth(units_to_depth(ui.alarmDepthDoubleSpinBox->value()));
	deviceDetails->setAlarmTimeEnabled(ui.alarmTimeCheckBox->isChecked());
	deviceDetails->setAlarmTime(ui.alarmTimeSpinBox->value());
}

void ConfigureDiveComputerDialog::readSettings()
{
	ui.statusLabel->clear();
	ui.errorLabel->clear();

	getDeviceData();
	config->readSettings(&device_data);
}

void ConfigureDiveComputerDialog::configMessage(QString msg)
{
	ui.statusLabel->setText(msg);
}

void ConfigureDiveComputerDialog::configError(QString err)
{
	ui.statusLabel->setText("");
	ui.errorLabel->setText(err);
}

void ConfigureDiveComputerDialog::getDeviceData()
{
	device_data.devname = strdup(ui.device->currentText().toUtf8().data());
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
	ui.statusLabel->setText(tr("Dive computer details read successfully."));
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
	// Enable the buttons to do operations on this data
	ui.saveSettingsPushButton->setEnabled(true);
	ui.backupButton->setEnabled(true);

	switch(ui.dcStackedWidget->currentIndex()) {
	case 0:
		reloadValuesOSTC3();
		break;
	case 1:
		reloadValuesSuuntoVyper();
		break;
	}
}

void ConfigureDiveComputerDialog::reloadValuesOSTC3()
{
	ui.serialNoLineEdit->setText(deviceDetails->serialNo());
	ui.firmwareVersionLineEdit->setText(deviceDetails->firmwareVersion());
	ui.customTextLlineEdit->setText(deviceDetails->customText());
	ui.diveModeComboBox->setCurrentIndex(deviceDetails->diveMode());
	ui.saturationSpinBox->setValue(deviceDetails->saturation());
	ui.desaturationSpinBox->setValue(deviceDetails->desaturation());
	ui.lastDecoSpinBox->setValue(deviceDetails->lastDeco());
	ui.brightnessComboBox->setCurrentIndex(deviceDetails->brightness());
	ui.unitsComboBox->setCurrentIndex(deviceDetails->units());
	ui.samplingRateComboBox->setCurrentIndex(deviceDetails->samplingRate());
	ui.salinitySpinBox->setValue(deviceDetails->salinity());
	ui.diveModeColour->setCurrentIndex(deviceDetails->diveModeColor());
	ui.languageComboBox->setCurrentIndex(deviceDetails->language());
	ui.dateFormatComboBox->setCurrentIndex(deviceDetails->dateFormat());
	ui.compassGainComboBox->setCurrentIndex(deviceDetails->compassGain());
	ui.safetyStopCheckBox->setChecked(deviceDetails->safetyStop());
	ui.gfHighSpinBox->setValue(deviceDetails->gfHigh());
	ui.gfLowSpinBox->setValue(deviceDetails->gfLow());
	ui.pressureSensorOffsetSpinBox->setValue(deviceDetails->pressureSensorOffset());
	ui.ppO2MinSpinBox->setValue(deviceDetails->ppO2Min());
	ui.ppO2MaxSpinBox->setValue(deviceDetails->ppO2Max());
	ui.futureTTSSpinBox->setValue(deviceDetails->futureTTS());
	ui.ccrModeComboBox->setCurrentIndex(deviceDetails->ccrMode());
	ui.decoTypeComboBox->setCurrentIndex(deviceDetails->decoType());
	ui.aGFSelectableCheckBox->setChecked(deviceDetails->aGFSelectable());
	ui.aGFHighSpinBox->setValue(deviceDetails->aGFHigh());
	ui.aGFLowSpinBox->setValue(deviceDetails->aGFLow());
	ui.calibrationGasSpinBox->setValue(deviceDetails->calibrationGas());
	ui.flipScreenCheckBox->setChecked(deviceDetails->flipScreen());
	ui.setPointFallbackCheckBox->setChecked(deviceDetails->setPointFallback());

	//load gas 1 values
	ui.ostc3GasTable->setItem(0,1, new QTableWidgetItem(QString::number(deviceDetails->gas1().oxygen)));
	ui.ostc3GasTable->setItem(0,2, new QTableWidgetItem(QString::number(deviceDetails->gas1().helium)));
	ui.ostc3GasTable->setItem(0,3, new QTableWidgetItem(QString::number(deviceDetails->gas1().type)));
	ui.ostc3GasTable->setItem(0,4, new QTableWidgetItem(QString::number(deviceDetails->gas1().depth)));

	//load gas 2 values
	ui.ostc3GasTable->setItem(1,1, new QTableWidgetItem(QString::number(deviceDetails->gas2().oxygen)));
	ui.ostc3GasTable->setItem(1,2, new QTableWidgetItem(QString::number(deviceDetails->gas2().helium)));
	ui.ostc3GasTable->setItem(1,3, new QTableWidgetItem(QString::number(deviceDetails->gas2().type)));
	ui.ostc3GasTable->setItem(1,4, new QTableWidgetItem(QString::number(deviceDetails->gas2().depth)));

	//load gas 3 values
	ui.ostc3GasTable->setItem(2,1, new QTableWidgetItem(QString::number(deviceDetails->gas3().oxygen)));
	ui.ostc3GasTable->setItem(2,2, new QTableWidgetItem(QString::number(deviceDetails->gas3().helium)));
	ui.ostc3GasTable->setItem(2,3, new QTableWidgetItem(QString::number(deviceDetails->gas3().type)));
	ui.ostc3GasTable->setItem(2,4, new QTableWidgetItem(QString::number(deviceDetails->gas3().depth)));

	//load gas 4 values
	ui.ostc3GasTable->setItem(3,1, new QTableWidgetItem(QString::number(deviceDetails->gas4().oxygen)));
	ui.ostc3GasTable->setItem(3,2, new QTableWidgetItem(QString::number(deviceDetails->gas4().helium)));
	ui.ostc3GasTable->setItem(3,3, new QTableWidgetItem(QString::number(deviceDetails->gas4().type)));
	ui.ostc3GasTable->setItem(3,4, new QTableWidgetItem(QString::number(deviceDetails->gas4().depth)));

	//load gas 5 values
	ui.ostc3GasTable->setItem(4,1, new QTableWidgetItem(QString::number(deviceDetails->gas5().oxygen)));
	ui.ostc3GasTable->setItem(4,2, new QTableWidgetItem(QString::number(deviceDetails->gas5().helium)));
	ui.ostc3GasTable->setItem(4,3, new QTableWidgetItem(QString::number(deviceDetails->gas5().type)));
	ui.ostc3GasTable->setItem(4,4, new QTableWidgetItem(QString::number(deviceDetails->gas5().depth)));

	//load dil 1 values
	ui.ostc3DilTable->setItem(0,1, new QTableWidgetItem(QString::number(deviceDetails->dil1().oxygen)));
	ui.ostc3DilTable->setItem(0,2, new QTableWidgetItem(QString::number(deviceDetails->dil1().helium)));
	ui.ostc3DilTable->setItem(0,3, new QTableWidgetItem(QString::number(deviceDetails->dil1().type)));
	ui.ostc3DilTable->setItem(0,4, new QTableWidgetItem(QString::number(deviceDetails->dil1().depth)));

	//load dil 2 values
	ui.ostc3DilTable->setItem(1,1, new QTableWidgetItem(QString::number(deviceDetails->dil2().oxygen)));
	ui.ostc3DilTable->setItem(1,2, new QTableWidgetItem(QString::number(deviceDetails->dil2().helium)));
	ui.ostc3DilTable->setItem(1,3, new QTableWidgetItem(QString::number(deviceDetails->dil2().type)));
	ui.ostc3DilTable->setItem(1,4, new QTableWidgetItem(QString::number(deviceDetails->dil2().depth)));

	//load dil 3 values
	ui.ostc3DilTable->setItem(2,1, new QTableWidgetItem(QString::number(deviceDetails->dil3().oxygen)));
	ui.ostc3DilTable->setItem(2,2, new QTableWidgetItem(QString::number(deviceDetails->dil3().helium)));
	ui.ostc3DilTable->setItem(2,3, new QTableWidgetItem(QString::number(deviceDetails->dil3().type)));
	ui.ostc3DilTable->setItem(2,4, new QTableWidgetItem(QString::number(deviceDetails->dil3().depth)));

	//load dil 4 values
	ui.ostc3DilTable->setItem(3,1, new QTableWidgetItem(QString::number(deviceDetails->dil4().oxygen)));
	ui.ostc3DilTable->setItem(3,2, new QTableWidgetItem(QString::number(deviceDetails->dil4().helium)));
	ui.ostc3DilTable->setItem(3,3, new QTableWidgetItem(QString::number(deviceDetails->dil4().type)));
	ui.ostc3DilTable->setItem(3,4, new QTableWidgetItem(QString::number(deviceDetails->dil4().depth)));

	//load dil 5 values
	ui.ostc3DilTable->setItem(4,1, new QTableWidgetItem(QString::number(deviceDetails->dil5().oxygen)));
	ui.ostc3DilTable->setItem(4,2, new QTableWidgetItem(QString::number(deviceDetails->dil5().helium)));
	ui.ostc3DilTable->setItem(4,3, new QTableWidgetItem(QString::number(deviceDetails->dil5().type)));
	ui.ostc3DilTable->setItem(4,4, new QTableWidgetItem(QString::number(deviceDetails->dil5().depth)));

	//load set point 1 values
	ui.ostc3SetPointTable->setItem(0, 1, new QTableWidgetItem(QString::number(deviceDetails->sp1().sp)));
	ui.ostc3SetPointTable->setItem(0, 2, new QTableWidgetItem(QString::number(deviceDetails->sp1().depth)));

	//load set point 2 values
	ui.ostc3SetPointTable->setItem(1, 1, new QTableWidgetItem(QString::number(deviceDetails->sp2().sp)));
	ui.ostc3SetPointTable->setItem(1, 2, new QTableWidgetItem(QString::number(deviceDetails->sp2().depth)));

	//load set point 3 values
	ui.ostc3SetPointTable->setItem(2, 1, new QTableWidgetItem(QString::number(deviceDetails->sp3().sp)));
	ui.ostc3SetPointTable->setItem(2, 2, new QTableWidgetItem(QString::number(deviceDetails->sp3().depth)));

	//load set point 4 values
	ui.ostc3SetPointTable->setItem(3, 1, new QTableWidgetItem(QString::number(deviceDetails->sp4().sp)));
	ui.ostc3SetPointTable->setItem(3, 2, new QTableWidgetItem(QString::number(deviceDetails->sp4().depth)));

	//load set point 5 values
	ui.ostc3SetPointTable->setItem(4, 1, new QTableWidgetItem(QString::number(deviceDetails->sp5().sp)));
	ui.ostc3SetPointTable->setItem(4, 2, new QTableWidgetItem(QString::number(deviceDetails->sp5().depth)));
}

void ConfigureDiveComputerDialog::reloadValuesSuuntoVyper()
{
	const char *depth_unit;
	ui.maxDepthDoubleSpinBox->setValue(get_depth_units(deviceDetails->maxDepth(), NULL, &depth_unit));
	ui.maxDepthDoubleSpinBox->setSuffix(depth_unit);
	ui.totalTimeSpinBox->setValue(deviceDetails->totalTime());
	ui.numberOfDivesSpinBox->setValue(deviceDetails->numberOfDives());
	ui.modelLineEdit->setText(deviceDetails->model());
	ui.firmwareVersionLineEdit_1->setText(deviceDetails->firmwareVersion());
	ui.serialNoLineEdit_1->setText(deviceDetails->serialNo());
	ui.customTextLlineEdit_1->setText(deviceDetails->customText());
	ui.samplingRateComboBox_1->setCurrentIndex(deviceDetails->samplingRate() == 60 ? 3 : (deviceDetails->samplingRate() / 10) - 1);
	ui.altitudeRangeComboBox->setCurrentIndex(deviceDetails->altitude());
	ui.personalSafetyComboBox->setCurrentIndex(deviceDetails->personalSafety());
	ui.timeFormatComboBox->setCurrentIndex(deviceDetails->timeFormat());
	ui.unitsComboBox_1->setCurrentIndex(deviceDetails->units());
	ui.diveModeComboBox_1->setCurrentIndex(deviceDetails->diveMode());
	ui.lightCheckBox->setChecked(deviceDetails->lightEnabled());
	ui.lightSpinBox->setValue(deviceDetails->light());
	ui.alarmDepthCheckBox->setChecked(deviceDetails->alarmDepthEnabled());
	ui.alarmDepthDoubleSpinBox->setValue(get_depth_units(deviceDetails->alarmDepth(), NULL, &depth_unit));
	ui.alarmDepthDoubleSpinBox->setSuffix(depth_unit);
	ui.alarmTimeCheckBox->setChecked(deviceDetails->alarmTimeEnabled());
	ui.alarmTimeSpinBox->setValue(deviceDetails->alarmTime());
}

void ConfigureDiveComputerDialog::on_backupButton_clicked()
{
	QString filename = existing_filename ?: prefs.default_filename;
	QFileInfo fi(filename);
	filename = fi.absolutePath().append(QDir::separator()).append("Backup.xml");
	QString backupPath = QFileDialog::getSaveFileName(this, tr("Backup dive computer settings"),
							  filename, tr("Backup files (*.xml)")
							  );
	if (!backupPath.isEmpty()) {
		populateDeviceDetails();
		getDeviceData();
		if (!config->saveXMLBackup(backupPath, deviceDetails, &device_data)) {
			QMessageBox::critical(this, tr("XML backup error"),
					      tr("An error occurred while saving the backup file.\n%1")
					      .arg(config->lastError)
					      );
		} else {
			QMessageBox::information(this, tr("Backup succeeded"),
						 tr("Your settings have been saved to: %1")
						 .arg(backupPath)
						 );
		}
	}
}

void ConfigureDiveComputerDialog::on_restoreBackupButton_clicked()
{
	QString filename = existing_filename ?: prefs.default_filename;
	QFileInfo fi(filename);
	filename = fi.absolutePath().append(QDir::separator()).append("Backup.xml");
	QString restorePath = QFileDialog::getOpenFileName(this, tr("Restore dive computer settings"),
							   filename, tr("Backup files (*.xml)")
							   );
	if (!restorePath.isEmpty()) {
		if (!config->restoreXMLBackup(restorePath, deviceDetails)) {
			QMessageBox::critical(this, tr("XML restore error"),
					      tr("An error occurred while restoring the backup file.\n%1")
					      .arg(config->lastError)
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
		config->startFirmwareUpdate(firmwarePath, &device_data);
	}
}

void ConfigureDiveComputerDialog::on_DiveComputerList_currentRowChanged(int currentRow)
{
	// Disable the buttons to do operations on this data
	ui.saveSettingsPushButton->setEnabled(false);
	ui.backupButton->setEnabled(false);

	switch (currentRow) {
	case 0:
		selected_vendor = "Heinrichs Weikamp";
		selected_product = "OSTC 3";
		ui.updateFirmwareButton->setEnabled(true);
		break;
	case 1:
		selected_vendor = "Suunto";
		selected_product = "Vyper";
		ui.updateFirmwareButton->setEnabled(false);
		break;
	default:
		/* Not Supported */
		return;
	}

	int dcType = DC_TYPE_SERIAL;


	if (selected_vendor == QString("Uemis"))
		dcType = DC_TYPE_UEMIS;
	fill_device_list(dcType);
}
