// SPDX-License-Identifier: GPL-2.0
#include "configuredivecomputer.h"
#include <QTextStream>
#include <QFile>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/tree.h>
#include <libxslt/transform.h>
#include <QStringList>
#include <QXmlStreamWriter>
#include "core/file.h"
#include "core/errorhelper.h"
#include "core/version.h"

ConfigureDiveComputer::ConfigureDiveComputer() : readThread(0),
	writeThread(0),
	resetThread(0),
	firmwareThread(0)
{
	setState(INITIAL);
}

void ConfigureDiveComputer::connectThreadSignals(DeviceThread *thread)
{
	connect(thread, &DeviceThread::finished, this, &ConfigureDiveComputer::readThreadFinished, Qt::QueuedConnection);
	connect(thread, &DeviceThread::error, this, &ConfigureDiveComputer::setError);
	connect(thread, &DeviceThread::progress, this, &ConfigureDiveComputer::progressEvent, Qt::QueuedConnection);
}

void ConfigureDiveComputer::readSettings(device_data_t *data)
{
	setState(READING);

	if (readThread)
		readThread->deleteLater();

	readThread = new ReadSettingsThread(this, data);
	connectThreadSignals(readThread);
	connect(readThread, &ReadSettingsThread::devicedetails, this, &ConfigureDiveComputer::deviceDetailsChanged);

	readThread->start();
}

void ConfigureDiveComputer::saveDeviceDetails(DeviceDetails *details, device_data_t *data)
{
	setState(WRITING);

	if (writeThread)
		writeThread->deleteLater();

	writeThread = new WriteSettingsThread(this, data);
	connectThreadSignals(writeThread);

	writeThread->setDeviceDetails(details);
	writeThread->start();
}

static QString writeGasDetails(gas g)
{
	return QStringList({
			QString::number(g.oxygen),
			QString::number(g.helium),
			QString::number(g.type),
			QString::number(g.depth)
		}).join(QLatin1Char(','));
}

bool ConfigureDiveComputer::saveXMLBackup(const QString &fileName, DeviceDetails *details, device_data_t *data)
{
	QString xml = "";
	QString vendor = data->vendor;
	QString product = data->product;
	QXmlStreamWriter writer(&xml);
	writer.setAutoFormatting(true);

	writer.writeStartDocument();
	writer.writeStartElement("DiveComputerSettingsBackup");
	writer.writeStartElement("DiveComputer");
	writer.writeTextElement("Vendor", vendor);
	writer.writeTextElement("Product", product);
	writer.writeEndElement();
	writer.writeStartElement("Settings");
	writer.writeTextElement("CustomText", details->customText);
	//Add gasses
	writer.writeTextElement("Gas1", writeGasDetails(details->gas1));
	writer.writeTextElement("Gas2", writeGasDetails(details->gas2));
	writer.writeTextElement("Gas3", writeGasDetails(details->gas3));
	writer.writeTextElement("Gas4", writeGasDetails(details->gas4));
	writer.writeTextElement("Gas5", writeGasDetails(details->gas5));
	//
	//Add dil values
	writer.writeTextElement("Dil1", writeGasDetails(details->dil1));
	writer.writeTextElement("Dil2", writeGasDetails(details->dil2));
	writer.writeTextElement("Dil3", writeGasDetails(details->dil3));
	writer.writeTextElement("Dil4", writeGasDetails(details->dil4));
	writer.writeTextElement("Dil5", writeGasDetails(details->dil5));

	//Add setpoint values
	QString sp1 = QString("%1,%2")
			      .arg(QString::number(details->sp1.sp),
				   QString::number(details->sp1.depth));
	QString sp2 = QString("%1,%2")
			      .arg(QString::number(details->sp2.sp),
				   QString::number(details->sp2.depth));
	QString sp3 = QString("%1,%2")
			      .arg(QString::number(details->sp3.sp),
				   QString::number(details->sp3.depth));
	QString sp4 = QString("%1,%2")
			      .arg(QString::number(details->sp4.sp),
				   QString::number(details->sp4.depth));
	QString sp5 = QString("%1,%2")
			      .arg(QString::number(details->sp5.sp),
				   QString::number(details->sp5.depth));
	writer.writeTextElement("SetPoint1", sp1);
	writer.writeTextElement("SetPoint2", sp2);
	writer.writeTextElement("SetPoint3", sp3);
	writer.writeTextElement("SetPoint4", sp4);
	writer.writeTextElement("SetPoint5", sp5);

	//Other Settings
	writer.writeTextElement("DiveMode", QString::number(details->diveMode));
	writer.writeTextElement("Saturation", QString::number(details->saturation));
	writer.writeTextElement("Desaturation", QString::number(details->desaturation));
	writer.writeTextElement("LastDeco", QString::number(details->lastDeco));
	writer.writeTextElement("Brightness", QString::number(details->brightness));
	writer.writeTextElement("Units", QString::number(details->units));
	writer.writeTextElement("SamplingRate", QString::number(details->samplingRate));
	writer.writeTextElement("Salinity", QString::number(details->salinity));
	writer.writeTextElement("DiveModeColor", QString::number(details->diveModeColor));
	writer.writeTextElement("Language", QString::number(details->language));
	writer.writeTextElement("DateFormat", QString::number(details->dateFormat));
	writer.writeTextElement("CompassGain", QString::number(details->compassGain));
	writer.writeTextElement("SafetyStop", QString::number(details->safetyStop));
	writer.writeTextElement("GfHigh", QString::number(details->gfHigh));
	writer.writeTextElement("GfLow", QString::number(details->gfLow));
	writer.writeTextElement("PressureSensorOffset", QString::number(details->pressureSensorOffset));
	writer.writeTextElement("PpO2Min", QString::number(details->ppO2Min));
	writer.writeTextElement("PpO2Max", QString::number(details->ppO2Max));
	writer.writeTextElement("FutureTTS", QString::number(details->futureTTS));
	writer.writeTextElement("CcrMode", QString::number(details->ccrMode));
	writer.writeTextElement("DecoType", QString::number(details->decoType));
	writer.writeTextElement("AGFSelectable", QString::number(details->aGFSelectable));
	writer.writeTextElement("AGFHigh", QString::number(details->aGFHigh));
	writer.writeTextElement("AGFLow", QString::number(details->aGFLow));
	writer.writeTextElement("CalibrationGas", QString::number(details->calibrationGas));
	writer.writeTextElement("FlipScreen", QString::number(details->flipScreen));
	writer.writeTextElement("SetPointFallback", QString::number(details->setPointFallback));
	writer.writeTextElement("LeftButtonSensitivity", QString::number(details->leftButtonSensitivity));
	writer.writeTextElement("RightButtonSensitivity", QString::number(details->rightButtonSensitivity));
	writer.writeTextElement("BottomGasConsumption", QString::number(details->bottomGasConsumption));
	writer.writeTextElement("DecoGasConsumption", QString::number(details->decoGasConsumption));
	writer.writeTextElement("ModWarning", QString::number(details->modWarning));
	writer.writeTextElement("DynamicAscendRate", QString::number(details->dynamicAscendRate));
	writer.writeTextElement("GraphicalSpeedIndicator", QString::number(details->graphicalSpeedIndicator));
	writer.writeTextElement("AlwaysShowppO2", QString::number(details->alwaysShowppO2));

	// Suunto vyper settings.
	writer.writeTextElement("Altitude", QString::number(details->altitude));
	writer.writeTextElement("PersonalSafety", QString::number(details->personalSafety));
	writer.writeTextElement("TimeFormat", QString::number(details->timeFormat));

	writer.writeStartElement("Light");
	writer.writeAttribute("enabled", QString::number(details->lightEnabled));
	writer.writeCharacters(QString::number(details->light));
	writer.writeEndElement();

	writer.writeStartElement("AlarmTime");
	writer.writeAttribute("enabled", QString::number(details->alarmTimeEnabled));
	writer.writeCharacters(QString::number(details->alarmTime));
	writer.writeEndElement();

	writer.writeStartElement("AlarmDepth");
	writer.writeAttribute("enabled", QString::number(details->alarmDepthEnabled));
	writer.writeCharacters(QString::number(details->alarmDepth));
	writer.writeEndElement();

	writer.writeEndElement();
	writer.writeEndElement();

	writer.writeEndDocument();
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly)) {
		lastError = tr("Could not save the backup file %1. Error Message: %2")
				    .arg(fileName, file.errorString());
		return false;
	}
	//file open successful. write data and save.
	QTextStream out(&file);
	out << xml;

	file.close();
	return true;
}

bool ConfigureDiveComputer::restoreXMLBackup(const QString &fileName, DeviceDetails *details)
{
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly)) {
		lastError = tr("Could not open backup file: %1").arg(file.errorString());
		return false;
	}

	QString xml = file.readAll();

	QXmlStreamReader reader(xml);
	while (!reader.atEnd()) {
		if (reader.isStartElement()) {
			QString settingName = reader.name().toString();
			QXmlStreamAttributes attributes = reader.attributes();
			reader.readNext();
			QString keyString = reader.text().toString();

			if (settingName == "CustomText")
				details->customText = keyString;

			if (settingName == "Gas1") {
				QStringList gasData = keyString.split(",");
				gas gas1;
				gas1.oxygen = gasData.at(0).toInt();
				gas1.helium = gasData.at(1).toInt();
				gas1.type = gasData.at(2).toInt();
				gas1.depth = gasData.at(3).toInt();
				details->gas1 = gas1;
			}

			if (settingName == "Gas2") {
				QStringList gasData = keyString.split(",");
				gas gas2;
				gas2.oxygen = gasData.at(0).toInt();
				gas2.helium = gasData.at(1).toInt();
				gas2.type = gasData.at(2).toInt();
				gas2.depth = gasData.at(3).toInt();
				details->gas2 = gas2;
			}

			if (settingName == "Gas3") {
				QStringList gasData = keyString.split(",");
				gas gas3;
				gas3.oxygen = gasData.at(0).toInt();
				gas3.helium = gasData.at(1).toInt();
				gas3.type = gasData.at(2).toInt();
				gas3.depth = gasData.at(3).toInt();
				details->gas3 = gas3;
			}

			if (settingName == "Gas4") {
				QStringList gasData = keyString.split(",");
				gas gas4;
				gas4.oxygen = gasData.at(0).toInt();
				gas4.helium = gasData.at(1).toInt();
				gas4.type = gasData.at(2).toInt();
				gas4.depth = gasData.at(3).toInt();
				details->gas4 = gas4;
			}

			if (settingName == "Gas5") {
				QStringList gasData = keyString.split(",");
				gas gas5;
				gas5.oxygen = gasData.at(0).toInt();
				gas5.helium = gasData.at(1).toInt();
				gas5.type = gasData.at(2).toInt();
				gas5.depth = gasData.at(3).toInt();
				details->gas5 = gas5;
			}

			if (settingName == "Dil1") {
				QStringList dilData = keyString.split(",");
				gas dil1;
				dil1.oxygen = dilData.at(0).toInt();
				dil1.helium = dilData.at(1).toInt();
				dil1.type = dilData.at(2).toInt();
				dil1.depth = dilData.at(3).toInt();
				details->dil1 = dil1;
			}

			if (settingName == "Dil2") {
				QStringList dilData = keyString.split(",");
				gas dil2;
				dil2.oxygen = dilData.at(0).toInt();
				dil2.helium = dilData.at(1).toInt();
				dil2.type = dilData.at(2).toInt();
				dil2.depth = dilData.at(3).toInt();
				details->dil1 = dil2;
			}

			if (settingName == "Dil3") {
				QStringList dilData = keyString.split(",");
				gas dil3;
				dil3.oxygen = dilData.at(0).toInt();
				dil3.helium = dilData.at(1).toInt();
				dil3.type = dilData.at(2).toInt();
				dil3.depth = dilData.at(3).toInt();
				details->dil3 = dil3;
			}

			if (settingName == "Dil4") {
				QStringList dilData = keyString.split(",");
				gas dil4;
				dil4.oxygen = dilData.at(0).toInt();
				dil4.helium = dilData.at(1).toInt();
				dil4.type = dilData.at(2).toInt();
				dil4.depth = dilData.at(3).toInt();
				details->dil4 = dil4;
			}

			if (settingName == "Dil5") {
				QStringList dilData = keyString.split(",");
				gas dil5;
				dil5.oxygen = dilData.at(0).toInt();
				dil5.helium = dilData.at(1).toInt();
				dil5.type = dilData.at(2).toInt();
				dil5.depth = dilData.at(3).toInt();
				details->dil5 = dil5;
			}

			if (settingName == "SetPoint1") {
				QStringList spData = keyString.split(",");
				setpoint sp1;
				sp1.sp = spData.at(0).toInt();
				sp1.depth = spData.at(1).toInt();
				details->sp1 = sp1;
			}

			if (settingName == "SetPoint2") {
				QStringList spData = keyString.split(",");
				setpoint sp2;
				sp2.sp = spData.at(0).toInt();
				sp2.depth = spData.at(1).toInt();
				details->sp2 = sp2;
			}

			if (settingName == "SetPoint3") {
				QStringList spData = keyString.split(",");
				setpoint sp3;
				sp3.sp = spData.at(0).toInt();
				sp3.depth = spData.at(1).toInt();
				details->sp3 = sp3;
			}

			if (settingName == "SetPoint4") {
				QStringList spData = keyString.split(",");
				setpoint sp4;
				sp4.sp = spData.at(0).toInt();
				sp4.depth = spData.at(1).toInt();
				details->sp4 = sp4;
			}

			if (settingName == "SetPoint5") {
				QStringList spData = keyString.split(",");
				setpoint sp5;
				sp5.sp = spData.at(0).toInt();
				sp5.depth = spData.at(1).toInt();
				details->sp5 = sp5;
			}

			if (settingName == "Saturation")
				details->saturation = keyString.toInt();

			if (settingName == "Desaturation")
				details->desaturation = keyString.toInt();

			if (settingName == "DiveMode")
				details->diveMode = keyString.toInt();

			if (settingName == "LastDeco")
				details->lastDeco = keyString.toInt();

			if (settingName == "Brightness")
				details->brightness = keyString.toInt();

			if (settingName == "Units")
				details->units = keyString.toInt();

			if (settingName == "SamplingRate")
				details->samplingRate = keyString.toInt();

			if (settingName == "Salinity")
				details->salinity = keyString.toInt();

			if (settingName == "DiveModeColour")
				details->diveModeColor = keyString.toInt();

			if (settingName == "Language")
				details->language = keyString.toInt();

			if (settingName == "DateFormat")
				details->dateFormat = keyString.toInt();

			if (settingName == "CompassGain")
				details->compassGain = keyString.toInt();

			if (settingName == "SafetyStop")
				details->safetyStop = keyString.toInt();

			if (settingName == "GfHigh")
				details->gfHigh = keyString.toInt();

			if (settingName == "GfLow")
				details->gfLow = keyString.toInt();

			if (settingName == "PressureSensorOffset")
				details->pressureSensorOffset = keyString.toInt();

			if (settingName == "PpO2Min")
				details->ppO2Min = keyString.toInt();

			if (settingName == "PpO2Max")
				details->ppO2Max = keyString.toInt();

			if (settingName == "FutureTTS")
				details->futureTTS = keyString.toInt();

			if (settingName == "CcrMode")
				details->ccrMode = keyString.toInt();

			if (settingName == "DecoType")
				details->decoType = keyString.toInt();

			if (settingName == "AGFSelectable")
				details->aGFSelectable = keyString.toInt();

			if (settingName == "AGFHigh")
				details->aGFHigh = keyString.toInt();

			if (settingName == "AGFLow")
				details->aGFLow = keyString.toInt();

			if (settingName == "CalibrationGas")
				details->calibrationGas = keyString.toInt();

			if (settingName == "FlipScreen")
				details->flipScreen = keyString.toInt();

			if (settingName == "SetPointFallback")
				details->setPointFallback = keyString.toInt();

			if (settingName == "LeftButtonSensitivity")
				details->leftButtonSensitivity = keyString.toInt();

			if (settingName == "RightButtonSensitivity")
				details->rightButtonSensitivity = keyString.toInt();

			if (settingName == "BottomGasConsumption")
				details->bottomGasConsumption = keyString.toInt();

			if (settingName == "DecoGasConsumption")
				details->decoGasConsumption = keyString.toInt();

			if (settingName == "ModWarning")
				details->modWarning = keyString.toInt();

			if (settingName == "DynamicAscendRate")
				details->dynamicAscendRate = keyString.toInt();

			if (settingName == "GraphicalSpeedIndicator")
				details->graphicalSpeedIndicator = keyString.toInt();

			if (settingName == "AlwaysShowppO2")
				details->alwaysShowppO2 = keyString.toInt();

			if (settingName == "Altitude")
				details->altitude = keyString.toInt();

			if (settingName == "PersonalSafety")
				details->personalSafety = keyString.toInt();

			if (settingName == "TimeFormat")
				details->timeFormat = keyString.toInt();

			if (settingName == "Light") {
				if (attributes.hasAttribute("enabled"))
					details->lightEnabled = attributes.value("enabled").toString().toInt();
				details->light = keyString.toInt();
			}

			if (settingName == "AlarmDepth") {
				if (attributes.hasAttribute("enabled"))
					details->alarmDepthEnabled = attributes.value("enabled").toString().toInt();
				details->alarmDepth = keyString.toInt();
			}

			if (settingName == "AlarmTime") {
				if (attributes.hasAttribute("enabled"))
					details->alarmTimeEnabled = attributes.value("enabled").toString().toInt();
				details->alarmTime = keyString.toInt();
			}
		}
		reader.readNext();
	}

	return true;
}

void ConfigureDiveComputer::startFirmwareUpdate(const QString &fileName, device_data_t *data, bool forceUpdate)
{
	setState(FWUPDATE);
	if (firmwareThread)
		firmwareThread->deleteLater();

	firmwareThread = new FirmwareUpdateThread(this, data, fileName, forceUpdate);
	connectThreadSignals(firmwareThread);

	firmwareThread->start();
}

void ConfigureDiveComputer::resetSettings(device_data_t *data)
{
	setState(RESETTING);

	if (resetThread)
		resetThread->deleteLater();

	resetThread = new ResetSettingsThread(this, data);
	connectThreadSignals(resetThread);

	resetThread->start();
}

void ConfigureDiveComputer::progressEvent(int percent)
{
	emit progress(percent);
}

void ConfigureDiveComputer::setState(ConfigureDiveComputer::states newState)
{
	currentState = newState;
	emit stateChanged(currentState);
}

void ConfigureDiveComputer::setError(QString err)
{
	lastError = err;
	emit error(std::move(err));
}

void ConfigureDiveComputer::readThreadFinished()
{
	setState(DONE);
	if (lastError.isEmpty()) {
		//No error
		emit message(tr("Dive computer details read successfully"));
	}
}

void ConfigureDiveComputer::writeThreadFinished()
{
	setState(DONE);
	if (lastError.isEmpty()) {
		//No error
		emit message(tr("Setting successfully written to device"));
	}
}

void ConfigureDiveComputer::firmwareThreadFinished()
{
	setState(DONE);
	if (lastError.isEmpty()) {
		//No error
		emit message(tr("Device firmware successfully updated"));
	}
}

void ConfigureDiveComputer::resetThreadFinished()
{
	setState(DONE);
	if (lastError.isEmpty()) {
		//No error
		emit message(tr("Device settings successfully reset"));
	}
}

QString ConfigureDiveComputer::dc_open(device_data_t *data)
{
	FILE *fp = NULL;
	dc_status_t rc;

	if (data->libdc_log)
		fp = subsurface_fopen(logfile_name, "w");

	data->libdc_logfile = fp;

	rc = dc_context_new(&data->context);
	if (rc != DC_STATUS_SUCCESS) {
		return tr("Unable to create libdivecomputer context");
	}

	if (fp) {
		dc_context_set_loglevel(data->context, DC_LOGLEVEL_ALL);
		dc_context_set_logfunc(data->context, logfunc, fp);
		fprintf(data->libdc_logfile, "Subsurface: v%s, ", subsurface_git_version());
		fprintf(data->libdc_logfile, "built with libdivecomputer v%s\n", dc_version(NULL));
	}

	rc = divecomputer_device_open(data);

	if (rc != DC_STATUS_SUCCESS) {
		report_error(errmsg(rc));
	} else {
		rc = dc_device_open(&data->device, data->context, data->descriptor, data->iostream);
	}

	if (rc != DC_STATUS_SUCCESS) {
		return tr("Could not a establish connection to the dive computer.");
	}

	setState(OPEN);

	return NULL;
}

void ConfigureDiveComputer::dc_close(device_data_t *data)
{
	if (data->device)
		dc_device_close(data->device);
	data->device = NULL;
	if (data->context)
		dc_context_free(data->context);
	data->context = NULL;
	dc_iostream_close(data->iostream);
	data->iostream = NULL;

	if (data->libdc_logfile)
		fclose(data->libdc_logfile);

	setState(INITIAL);
}
