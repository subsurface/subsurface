#include "configuredivecomputer.h"
#include "libdivecomputer/hw.h"
#include <QDebug>
#include <QFile>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/tree.h>
#include <libxslt/transform.h>
#include <QStringList>
#include <QXmlStreamWriter>

ConfigureDiveComputer::ConfigureDiveComputer(QObject *parent) :
	QObject(parent),
	readThread(0),
	writeThread(0),
	resetThread(0),
	firmwareThread(0)
{
	setState(INITIAL);
}

void ConfigureDiveComputer::readSettings(device_data_t *data)
{
	setState(READING);

	if (readThread)
		readThread->deleteLater();

	readThread = new ReadSettingsThread(this, data);
	connect(readThread, SIGNAL(finished()),
		this, SLOT(readThreadFinished()), Qt::QueuedConnection);
	connect(readThread, SIGNAL(error(QString)), this, SLOT(setError(QString)));
	connect(readThread, SIGNAL(devicedetails(DeviceDetails*)), this,
		SIGNAL(deviceDetailsChanged(DeviceDetails*)));

	readThread->start();
}

void ConfigureDiveComputer::saveDeviceDetails(DeviceDetails *details, device_data_t *data)
{
	setState(WRITING);

	if (writeThread)
		writeThread->deleteLater();

	writeThread = new WriteSettingsThread(this, data);
	connect(writeThread, SIGNAL(finished()),
		this, SLOT(writeThreadFinished()), Qt::QueuedConnection);
	connect(writeThread, SIGNAL(error(QString)), this, SLOT(setError(QString)));

	writeThread->setDeviceDetails(details);
	writeThread->start();
}

bool ConfigureDiveComputer::saveXMLBackup(QString fileName, DeviceDetails *details, device_data_t *data)
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
	writer.writeTextElement("CustomText", details->customText());
	//Add gasses
	QString gas1 = QString("%1,%2,%3,%4")
			.arg(QString::number(details->gas1().oxygen),
			     QString::number(details->gas1().helium),
			     QString::number(details->gas1().type),
			     QString::number(details->gas1().depth)
			     );
	QString gas2 = QString("%1,%2,%3,%4")
			.arg(QString::number(details->gas2().oxygen),
			     QString::number(details->gas2().helium),
			     QString::number(details->gas2().type),
			     QString::number(details->gas2().depth)
			     );
	QString gas3 = QString("%1,%2,%3,%4")
			.arg(QString::number(details->gas3().oxygen),
			     QString::number(details->gas3().helium),
			     QString::number(details->gas3().type),
			     QString::number(details->gas3().depth)
			     );
	QString gas4 = QString("%1,%2,%3,%4")
			.arg(QString::number(details->gas4().oxygen),
			     QString::number(details->gas4().helium),
			     QString::number(details->gas4().type),
			     QString::number(details->gas4().depth)
			     );
	QString gas5 = QString("%1,%2,%3,%4")
			.arg(QString::number(details->gas5().oxygen),
			     QString::number(details->gas5().helium),
			     QString::number(details->gas5().type),
			     QString::number(details->gas5().depth)
			     );
	writer.writeTextElement("Gas1", gas1);
	writer.writeTextElement("Gas2", gas2);
	writer.writeTextElement("Gas3", gas3);
	writer.writeTextElement("Gas4", gas4);
	writer.writeTextElement("Gas5", gas5);
	//
	//Add dil values
	QString dil1 = QString("%1,%2,%3,%4")
			.arg(QString::number(details->dil1().oxygen),
			     QString::number(details->dil1().helium),
			     QString::number(details->dil1().type),
			     QString::number(details->dil1().depth)
			     );
	QString dil2 = QString("%1,%2,%3,%4")
			.arg(QString::number(details->dil2().oxygen),
			     QString::number(details->dil2().helium),
			     QString::number(details->dil2().type),
			     QString::number(details->dil2().depth)
			     );
	QString dil3 = QString("%1,%2,%3,%4")
			.arg(QString::number(details->dil3().oxygen),
			     QString::number(details->dil3().helium),
			     QString::number(details->dil3().type),
			     QString::number(details->dil3().depth)
			     );
	QString dil4 = QString("%1,%2,%3,%4")
			.arg(QString::number(details->dil4().oxygen),
			     QString::number(details->dil4().helium),
			     QString::number(details->dil4().type),
			     QString::number(details->dil4().depth)
			     );
	QString dil5 = QString("%1,%2,%3,%4")
			.arg(QString::number(details->dil5().oxygen),
			     QString::number(details->dil5().helium),
			     QString::number(details->dil5().type),
			     QString::number(details->dil5().depth)
			     );
	writer.writeTextElement("Dil1", dil1);
	writer.writeTextElement("Dil2", dil2);
	writer.writeTextElement("Dil3", dil3);
	writer.writeTextElement("Dil4", dil4);
	writer.writeTextElement("Dil5", dil5);
	//
	//Add set point values
	QString sp1 = QString("%1,%2")
			.arg(QString::number(details->sp1().sp),
			     QString::number(details->sp1().depth)
			     );
	QString sp2 = QString("%1,%2")
			.arg(QString::number(details->sp2().sp),
			     QString::number(details->sp2().depth)
			     );
	QString sp3 = QString("%1,%2")
			.arg(QString::number(details->sp3().sp),
			     QString::number(details->sp3().depth)
			     );
	QString sp4 = QString("%1,%2")
			.arg(QString::number(details->sp4().sp),
			     QString::number(details->sp4().depth)
			     );
	QString sp5 = QString("%1,%2")
			.arg(QString::number(details->sp5().sp),
			     QString::number(details->sp5().depth)
			     );
	writer.writeTextElement("SetPoint1", sp1);
	writer.writeTextElement("SetPoint2", sp2);
	writer.writeTextElement("SetPoint3", sp3);
	writer.writeTextElement("SetPoint4", sp4);
	writer.writeTextElement("SetPoint5", sp5);

	//Other Settings
	writer.writeTextElement("DiveMode", QString::number(details->diveMode()));
	writer.writeTextElement("Saturation", QString::number(details->saturation()));
	writer.writeTextElement("Desaturation", QString::number(details->desaturation()));
	writer.writeTextElement("LastDeco", QString::number(details->lastDeco()));
	writer.writeTextElement("Brightness", QString::number(details->brightness()));
	writer.writeTextElement("Units", QString::number(details->units()));
	writer.writeTextElement("SamplingRate", QString::number(details->samplingRate()));
	writer.writeTextElement("Salinity", QString::number(details->salinity()));
	writer.writeTextElement("DiveModeColor", QString::number(details->diveModeColor()));
	writer.writeTextElement("Language", QString::number(details->language()));
	writer.writeTextElement("DateFormat", QString::number(details->dateFormat()));
	writer.writeTextElement("CompassGain", QString::number(details->compassGain()));
	writer.writeTextElement("SafetyStop", QString::number(details->safetyStop()));
	writer.writeTextElement("GfHigh", QString::number(details->gfHigh()));
	writer.writeTextElement("GfLow", QString::number(details->gfLow()));
	writer.writeTextElement("PressureSensorOffset", QString::number(details->pressureSensorOffset()));
	writer.writeTextElement("PpO2Min", QString::number(details->ppO2Min()));
	writer.writeTextElement("PpO2Max", QString::number(details->ppO2Max()));
	writer.writeTextElement("FutureTTS", QString::number(details->futureTTS()));
	writer.writeTextElement("CcrMode", QString::number(details->ccrMode()));
	writer.writeTextElement("DecoType", QString::number(details->decoType()));
	writer.writeTextElement("AGFSelectable", QString::number(details->aGFSelectable()));
	writer.writeTextElement("AGFHigh", QString::number(details->aGFHigh()));
	writer.writeTextElement("AGFLow", QString::number(details->aGFLow()));
	writer.writeTextElement("CalibrationGas", QString::number(details->calibrationGas()));
	writer.writeTextElement("FlipScreen", QString::number(details->flipScreen()));
	writer.writeTextElement("SetPointFallback", QString::number(details->setPointFallback()));

	// Suunto vyper settings.
	writer.writeTextElement("Altitude", QString::number(details->altitude()));
	writer.writeTextElement("PersonalSafety", QString::number(details->personalSafety()));
	writer.writeTextElement("TimeFormat", QString::number(details->timeFormat()));

	writer.writeStartElement("Light");
	writer.writeAttribute("enabled", QString::number(details->lightEnabled()));
	writer.writeCharacters(QString::number(details->light()));
	writer.writeEndElement();

	writer.writeStartElement("AlarmTime");
	writer.writeAttribute("enabled", QString::number(details->alarmTimeEnabled()));
	writer.writeCharacters(QString::number(details->alarmTime()));
	writer.writeEndElement();

	writer.writeStartElement("AlarmDepth");
	writer.writeAttribute("enabled", QString::number(details->alarmDepthEnabled()));
	writer.writeCharacters(QString::number(details->alarmDepth()));
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

bool ConfigureDiveComputer::restoreXMLBackup(QString fileName, DeviceDetails *details)
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
				details->setCustomText(keyString);

			if (settingName == "Gas1") {
				QStringList gasData = keyString.split(",");
				gas gas1;
				gas1.oxygen = gasData.at(0).toInt();
				gas1.helium = gasData.at(1).toInt();
				gas1.type = gasData.at(2).toInt();
				gas1.depth = gasData.at(3).toInt();
				details->setGas1(gas1);
			}

			if (settingName == "Gas2") {
				QStringList gasData = keyString.split(",");
				gas gas2;
				gas2.oxygen = gasData.at(0).toInt();
				gas2.helium = gasData.at(1).toInt();
				gas2.type = gasData.at(2).toInt();
				gas2.depth = gasData.at(3).toInt();
				details->setGas2(gas2);
			}

			if (settingName == "Gas3") {
				QStringList gasData = keyString.split(",");
				gas gas3;
				gas3.oxygen = gasData.at(0).toInt();
				gas3.helium = gasData.at(1).toInt();
				gas3.type = gasData.at(2).toInt();
				gas3.depth = gasData.at(3).toInt();
				details->setGas3(gas3);
			}

			if (settingName == "Gas4") {
				QStringList gasData = keyString.split(",");
				gas gas4;
				gas4.oxygen = gasData.at(0).toInt();
				gas4.helium = gasData.at(1).toInt();
				gas4.type = gasData.at(2).toInt();
				gas4.depth = gasData.at(3).toInt();
				details->setGas4(gas4);
			}

			if (settingName == "Gas5") {
				QStringList gasData = keyString.split(",");
				gas gas5;
				gas5.oxygen = gasData.at(0).toInt();
				gas5.helium = gasData.at(1).toInt();
				gas5.type = gasData.at(2).toInt();
				gas5.depth = gasData.at(3).toInt();
				details->setGas5(gas5);
			}

			if (settingName == "Dil1") {
				QStringList dilData = keyString.split(",");
				gas dil1;
				dil1.oxygen = dilData.at(0).toInt();
				dil1.helium = dilData.at(1).toInt();
				dil1.type = dilData.at(2).toInt();
				dil1.depth = dilData.at(3).toInt();
				details->setDil1(dil1);
			}

			if (settingName == "Dil2") {
				QStringList dilData = keyString.split(",");
				gas dil2;
				dil2.oxygen = dilData.at(0).toInt();
				dil2.helium = dilData.at(1).toInt();
				dil2.type = dilData.at(2).toInt();
				dil2.depth = dilData.at(3).toInt();
				details->setDil1(dil2);
			}

			if (settingName == "Dil3") {
				QStringList dilData = keyString.split(",");
				gas dil3;
				dil3.oxygen = dilData.at(0).toInt();
				dil3.helium = dilData.at(1).toInt();
				dil3.type = dilData.at(2).toInt();
				dil3.depth = dilData.at(3).toInt();
				details->setDil3(dil3);
			}

			if (settingName == "Dil4") {
				QStringList dilData = keyString.split(",");
				gas dil4;
				dil4.oxygen = dilData.at(0).toInt();
				dil4.helium = dilData.at(1).toInt();
				dil4.type = dilData.at(2).toInt();
				dil4.depth = dilData.at(3).toInt();
				details->setDil4(dil4);
			}

			if (settingName == "Dil5") {
				QStringList dilData = keyString.split(",");
				gas dil5;
				dil5.oxygen = dilData.at(0).toInt();
				dil5.helium = dilData.at(1).toInt();
				dil5.type = dilData.at(2).toInt();
				dil5.depth = dilData.at(3).toInt();
				details->setDil5(dil5);
			}

			if (settingName == "SetPoint1") {
				QStringList spData = keyString.split(",");
				setpoint sp1;
				sp1.sp = spData.at(0).toInt();
				sp1.depth = spData.at(1).toInt();
				details->setSp1(sp1);
			}

			if (settingName == "SetPoint2") {
				QStringList spData = keyString.split(",");
				setpoint sp2;
				sp2.sp = spData.at(0).toInt();
				sp2.depth = spData.at(1).toInt();
				details->setSp2(sp2);
			}

			if (settingName == "SetPoint3") {
				QStringList spData = keyString.split(",");
				setpoint sp3;
				sp3.sp = spData.at(0).toInt();
				sp3.depth = spData.at(1).toInt();
				details->setSp3(sp3);
			}

			if (settingName == "SetPoint4") {
				QStringList spData = keyString.split(",");
				setpoint sp4;
				sp4.sp = spData.at(0).toInt();
				sp4.depth = spData.at(1).toInt();
				details->setSp4(sp4);
			}

			if (settingName == "SetPoint5") {
				QStringList spData = keyString.split(",");
				setpoint sp5;
				sp5.sp = spData.at(0).toInt();
				sp5.depth = spData.at(1).toInt();
				details->setSp5(sp5);
			}

			if (settingName == "Saturation")
				details->setSaturation(keyString.toInt());

			if (settingName == "Desaturation")
				details->setDesaturation(keyString.toInt());

			if (settingName == "DiveMode")
				details->setDiveMode(keyString.toInt());

			if (settingName == "LastDeco")
				details->setLastDeco(keyString.toInt());

			if (settingName == "Brightness")
				details->setBrightness(keyString.toInt());

			if (settingName == "Units")
				details->setUnits(keyString.toInt());

			if (settingName == "SamplingRate")
				details->setSamplingRate(keyString.toInt());

			if (settingName == "Salinity")
				details->setSalinity(keyString.toInt());

			if (settingName == "DiveModeColour")
				details->setDiveModeColor(keyString.toInt());

			if (settingName == "Language")
				details->setLanguage(keyString.toInt());

			if (settingName == "DateFormat")
				details->setDateFormat(keyString.toInt());

			if (settingName == "CompassGain")
				details->setCompassGain(keyString.toInt());

			if (settingName == "SafetyStop")
				details->setSafetyStop(keyString.toInt());

			if (settingName == "GfHigh")
				details->setGfHigh(keyString.toInt());

			if (settingName == "GfLow")
				details->setGfLow(keyString.toInt());

			if (settingName == "PressureSensorOffset")
				details->setPressureSensorOffset(keyString.toInt());

			if (settingName == "PpO2Min")
				details->setPpO2Min(keyString.toInt());

			if (settingName == "PpO2Max")
				details->setPpO2Max(keyString.toInt());

			if (settingName == "FutureTTS")
				details->setFutureTTS(keyString.toInt());

			if (settingName == "CcrMode")
				details->setCcrMode(keyString.toInt());

			if (settingName == "DecoType")
				details->setDecoType(keyString.toInt());

			if (settingName == "AGFSelectable")
				details->setAGFSelectable(keyString.toInt());

			if (settingName == "AGFHigh")
				details->setAGFHigh(keyString.toInt());

			if (settingName == "AGFLow")
				details->setAGFLow(keyString.toInt());

			if (settingName == "CalibrationGas")
				details->setCalibrationGas(keyString.toInt());

			if (settingName == "FlipScreen")
				details->setFlipScreen(keyString.toInt());

			if (settingName == "SetPointFallback")
				details->setSetPointFallback(keyString.toInt());

			if (settingName == "Altitude")
				details->setAltitude(keyString.toInt());

			if (settingName == "PersonalSafety")
				details->setPersonalSafety(keyString.toInt());

			if (settingName == "TimeFormat")
				details->setTimeFormat(keyString.toInt());

			if (settingName == "Light") {
				if (attributes.hasAttribute("enabled"))
					details->setLightEnabled(attributes.value("enabled").toString().toInt());
				details->setLight(keyString.toInt());
			}

			if (settingName == "AlarmDepth") {
				if (attributes.hasAttribute("enabled"))
					details->setAlarmDepthEnabled(attributes.value("enabled").toString().toInt());
				details->setAlarmDepth(keyString.toInt());
			}

			if (settingName == "AlarmTime") {
				if (attributes.hasAttribute("enabled"))
					details->setAlarmTimeEnabled(attributes.value("enabled").toString().toInt());
				details->setAlarmTime(keyString.toInt());
			}
		}
		reader.readNext();
	}

	return true;
}

void ConfigureDiveComputer::startFirmwareUpdate(QString fileName, device_data_t *data)
{
	setState(FWUPDATE);

	if (firmwareThread)
		firmwareThread->deleteLater();

	firmwareThread = new FirmwareUpdateThread(this, data, fileName);
	connect(firmwareThread, SIGNAL(finished()),
		this, SLOT(firmwareThreadFinished()), Qt::QueuedConnection);
	connect(firmwareThread, SIGNAL(error(QString)), this, SLOT(setError(QString)));

	firmwareThread->start();
}

void ConfigureDiveComputer::resetSettings(device_data_t *data)
{
	setState(RESETTING);

	if (resetThread)
		resetThread->deleteLater();

	resetThread = new ResetSettingsThread(this, data);
	connect(resetThread, SIGNAL(finished()),
		this, SLOT(resetThreadFinished()), Qt::QueuedConnection);
	connect(resetThread, SIGNAL(error(QString)), this, SLOT(setError(QString)));

	resetThread->start();
}

void ConfigureDiveComputer::setState(ConfigureDiveComputer::states newState)
{
	currentState = newState;
	emit stateChanged(currentState);
}

void ConfigureDiveComputer::setError(QString err)
{
	lastError = err;
	emit error(err);
}

void ConfigureDiveComputer::readThreadFinished()
{
	setState(DONE);
	emit readFinished();
}

void ConfigureDiveComputer::writeThreadFinished()
{
	setState(DONE);
	if (writeThread->lastError.isEmpty()) {
		//No error
		emit message(tr("Setting successfully written to device"));
	}
}

void ConfigureDiveComputer::firmwareThreadFinished()
{
	setState(DONE);
	if (firmwareThread->lastError.isEmpty()) {
		//No error
		emit message(tr("Device firmware successfully updated"));
	}
}

void ConfigureDiveComputer::resetThreadFinished()
{
	setState(DONE);
	if (resetThread->lastError.isEmpty()) {
		//No error
		emit message(tr("Device settings successfully reset"));
	}
}
