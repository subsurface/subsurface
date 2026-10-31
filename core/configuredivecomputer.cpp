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

ConfigureDiveComputer::~ConfigureDiveComputer()
{
	stopReadSession();
	for (DeviceThread *thread : { static_cast<DeviceThread *>(writeThread), static_cast<DeviceThread *>(resetThread), static_cast<DeviceThread *>(firmwareThread) })
		if (thread)
			thread->wait();
}

void ConfigureDiveComputer::stopReadSession()
{
	if (!readThread)
		return;
	disconnect(readThread, nullptr, this, nullptr);
	readThread->stopSession();
	readThread->wait();
	delete readThread;
	readThread = nullptr;
}

void ConfigureDiveComputer::connectThreadSignals(DeviceThread *thread)
{
	if (thread != readThread)
		connect(thread, &DeviceThread::finished, this, &ConfigureDiveComputer::threadFinished, Qt::QueuedConnection);
	connect(thread, &DeviceThread::error, this, &ConfigureDiveComputer::setError);
	connect(thread, &DeviceThread::progress, this, &ConfigureDiveComputer::progressEvent, Qt::QueuedConnection);
}

void ConfigureDiveComputer::setReadTransportForTesting(std::function<QString(device_data_t *)> open,
	std::function<dc_status_t(device_data_t *)> close, std::function<bool(device_data_t *)> operation)
{
	readOpen = std::move(open);
	readClose = std::move(close);
	readOperation = std::move(operation);
}

void ConfigureDiveComputer::readSettings(device_data_t *data)
{
	if (currentState == READING || currentState == WRITING) {
		emit error(tr("Wait for the settings operation to finish before reading again."));
		return;
	}
	lastError.clear();
	ostc4Snapshot.valid = false;
	stopReadSession();
	setState(READING);

	readThread = new ReadSettingsThread(this, data);
	// An OSTC4/5 may be selected as OSTC3 over Bluetooth. All OSTC3-family
	// reads therefore use a worker-owned session, rather than using the GUI's
	// Qt RFCOMM socket from ReadSettingsThread. Leave other families unchanged.
	if (data->descriptor && dc_descriptor_get_type(data->descriptor) == DC_FAMILY_HW_OSTC3) {
		// The connection is deferred until this worker. No GUI-owned socket is
		// handed to a QThread, including when OSTC4/5 was selected as OSTC3.
		// A reset/firmware operation may have opened a GUI session meanwhile.
		if (data->device || data->iostream || data->context)
			closeTransport(data);
		readThread->setTransport(readOpen, [this, thread = readThread](device_data_t *session) {
			if (readClose(session) != DC_STATUS_SUCCESS)
				emit thread->error(tr("Settings transport did not close cleanly. The connection may not be ready for another read; see any preceding save report for written values."));
		});
		readThread->setSession(true);
	}
	if (readOperation)
		readThread->setTestOperation(readOperation);
	if (readOperation)
		readThread->setTestDetails(testDetails);
	if (testIO)
		readThread->setSaveIO(testIO);
	connectThreadSignals(readThread);
	connect(readThread, &ReadSettingsThread::devicedetails, this, [this](DeviceDetails details) {
		ostc4Snapshot = details.ostc4Snapshot;
		emit deviceDetailsChanged(std::move(details));
	});
	connect(readThread, &ReadSettingsThread::result, this, &ConfigureDiveComputer::message);
	connect(readThread, &ReadSettingsThread::operationFinished, this, [this](bool writing) {
		if ((!writing && currentState != READING) || (writing && currentState != WRITING))
			return;
		const bool success = lastError.isEmpty();
		if (writing) {
			ostc4Snapshot.valid = false;
			ostc4Writing = false;
		}
		setState(success ? DONE : ERRORED);
		if (success && !writing && ostc4Snapshot.identity.model != 0x43 && ostc4Snapshot.identity.model != 0x44)
			emit message(tr("The operation completed successfully"));
	});

	readThread->start();
}

void ConfigureDiveComputer::saveDeviceDetails(const DeviceDetails &details, device_data_t *data)
{
	if (currentState == READING || currentState == WRITING) {
		emit error(tr("Wait for the settings operation to finish before saving."));
		return;
	}
	ostc4Writing = data->product == "OSTC 4" || data->product == "OSTC 5" ||
		ostc4Snapshot.identity.model == 0x43 || ostc4Snapshot.identity.model == 0x44;
	if (ostc4Writing && !ostc4Snapshot.valid) {
		ostc4Writing = false;
		setError(tr("OSTC 4/5 settings are unavailable or need a fresh read."));
		return;
	}
	const bool ostc3Family = data->descriptor && dc_descriptor_get_type(data->descriptor) == DC_FAMILY_HW_OSTC3;
	if (ostc4Writing && !ostc3Family) {
		ostc4Writing = false;
		setError(tr("OSTC 4/5 settings need a recognised device descriptor; reload required."));
		return;
	}
	if (ostc4Writing && (!readThread || !readThread->isRunning() || (!data->device && !readOperation))) {
		ostc4Writing = false;
		setError(tr("OSTC 4/5 settings session ended; reload required."));
		return;
	}
	lastError.clear();
	setState(WRITING);
	DeviceDetails checked = details;
	if (ostc4Writing) {
		checked.ostc4Snapshot = ostc4Snapshot;
		readThread->save(checked, ostc4BackupPath);
		return;
	}
	if (ostc3Family)
		stopReadSession();

	if (writeThread)
		writeThread->deleteLater();

	writeThread = new WriteSettingsThread(this, data);
	connectThreadSignals(writeThread);
	if (ostc3Family)
		writeThread->setTransport(ConfigureDiveComputer::openTransport, ConfigureDiveComputer::closeTransport);

	writeThread->setDeviceDetails(checked);
	writeThread->start();
}

bool ConfigureDiveComputer::validateOstc4Backup(const QString &path, QString &description) const
{
	return ostc4Snapshot.valid && Ostc4SettingsTransaction::validateBackup(path, ostc4Snapshot.identity, description);
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

bool ConfigureDiveComputer::saveXMLBackup(const QString &fileName, const DeviceDetails &details, device_data_t *data)
{
	QString xml = "";
	QXmlStreamWriter writer(&xml);
	writer.setAutoFormatting(true);

	writer.writeStartDocument();
	writer.writeStartElement("DiveComputerSettingsBackup");
	writer.writeStartElement("DiveComputer");
	writer.writeTextElement("Vendor", QString::fromStdString(data->vendor));
	writer.writeTextElement("Product", QString::fromStdString(data->product));
	writer.writeEndElement();
	writer.writeStartElement("Settings");
	writer.writeTextElement("CustomText", details.customText);
	//Add gasses
	writer.writeTextElement("Gas1", writeGasDetails(details.gas1));
	writer.writeTextElement("Gas2", writeGasDetails(details.gas2));
	writer.writeTextElement("Gas3", writeGasDetails(details.gas3));
	writer.writeTextElement("Gas4", writeGasDetails(details.gas4));
	writer.writeTextElement("Gas5", writeGasDetails(details.gas5));
	//
	//Add dil values
	writer.writeTextElement("Dil1", writeGasDetails(details.dil1));
	writer.writeTextElement("Dil2", writeGasDetails(details.dil2));
	writer.writeTextElement("Dil3", writeGasDetails(details.dil3));
	writer.writeTextElement("Dil4", writeGasDetails(details.dil4));
	writer.writeTextElement("Dil5", writeGasDetails(details.dil5));

	//Add setpoint values
	QString sp1 = QString("%1,%2")
			      .arg(QString::number(details.sp1.sp),
				   QString::number(details.sp1.depth));
	QString sp2 = QString("%1,%2")
			      .arg(QString::number(details.sp2.sp),
				   QString::number(details.sp2.depth));
	QString sp3 = QString("%1,%2")
			      .arg(QString::number(details.sp3.sp),
				   QString::number(details.sp3.depth));
	QString sp4 = QString("%1,%2")
			      .arg(QString::number(details.sp4.sp),
				   QString::number(details.sp4.depth));
	QString sp5 = QString("%1,%2")
			      .arg(QString::number(details.sp5.sp),
				   QString::number(details.sp5.depth));
	writer.writeTextElement("SetPoint1", sp1);
	writer.writeTextElement("SetPoint2", sp2);
	writer.writeTextElement("SetPoint3", sp3);
	writer.writeTextElement("SetPoint4", sp4);
	writer.writeTextElement("SetPoint5", sp5);

	//Other Settings
	writer.writeTextElement("DiveMode", QString::number(details.diveMode));
	writer.writeTextElement("LastDeco", QString::number(details.lastDeco));
	writer.writeTextElement("Brightness", QString::number(details.brightness));
	writer.writeTextElement("Units", QString::number(details.units));
	writer.writeTextElement("Salinity", QString::number(details.salinity));
	writer.writeTextElement("DiveModeColor", QString::number(details.diveModeColor));
	writer.writeTextElement("Language", QString::number(details.language));
	writer.writeTextElement("DateFormat", QString::number(details.dateFormat));
	writer.writeTextElement("CompassGain", QString::number(details.compassGain));
	writer.writeTextElement("SafetyStop", QString::number(details.safetyStop));
	writer.writeTextElement("GfHigh", QString::number(details.gfHigh));
	writer.writeTextElement("GfLow", QString::number(details.gfLow));
	writer.writeTextElement("PressureSensorOffset", QString::number(details.pressureSensorOffset));
	writer.writeTextElement("PpO2Min", QString::number(details.ppO2Min));
	writer.writeTextElement("PpO2Max", QString::number(details.ppO2Max));
	writer.writeTextElement("FutureTTS", QString::number(details.futureTTS));
	writer.writeTextElement("CcrMode", QString::number(details.ccrMode));
	writer.writeTextElement("DecoType", QString::number(details.decoType));
	writer.writeTextElement("AGFHigh", QString::number(details.aGFHigh));
	writer.writeTextElement("AGFLow", QString::number(details.aGFLow));
	writer.writeTextElement("FlipScreen", QString::number(details.flipScreen));
	writer.writeTextElement("SetPointFallback", QString::number(details.setPointFallback));
	writer.writeTextElement("AlwaysShowppO2", QString::number(details.alwaysShowppO2));
	writer.writeTextElement("DecoGasConsumption", QString::number(details.decoGasConsumption));

	writer.writeComment("Heinrichs Weikamp OSTC 4/5 only:");
	writer.writeTextElement("TravelGasConsumption", QString::number(details.travelGasConsumption));
	writer.writeTextElement("VPMConservatism", QString::number(details.vpmConservatism));
	writer.writeTextElement("ButtonSensitivity", QString::number(details.buttonSensitivity));
	writer.writeTextElement("ButtonBalance", QString::number(details.buttonBalance));

	writer.writeComment("Not used on Heinrichs Weikamp OSTC 4/5:");
	writer.writeTextElement("BottomGasConsumption", QString::number(details.bottomGasConsumption));
	writer.writeTextElement("Saturation", QString::number(details.saturation));
	writer.writeTextElement("Desaturation", QString::number(details.desaturation));
	writer.writeTextElement("SamplingRate", QString::number(details.samplingRate));
	writer.writeTextElement("AGFSelectable", QString::number(details.aGFSelectable));
	writer.writeTextElement("CalibrationGas", QString::number(details.calibrationGas));
	writer.writeTextElement("RightButtonSensitivity", QString::number(details.rightButtonSensitivity));
	writer.writeTextElement("LeftButtonSensitivity", QString::number(details.leftButtonSensitivity));
	writer.writeTextElement("ModWarning", QString::number(details.modWarning));
	writer.writeTextElement("DynamicAscendRate", QString::number(details.dynamicAscendRate));
	writer.writeTextElement("GraphicalSpeedIndicator", QString::number(details.graphicalSpeedIndicator));
	writer.writeTextElement("SafetyStopEndDepth", QString::number(details.safetyStopResetDepth));
	writer.writeTextElement("SafetyStopEndDepth", QString::number(details.safetyStopResetDepth));

	writer.writeComment("Suunto vyper specific settings:");
	writer.writeTextElement("Altitude", QString::number(details.altitude));
	writer.writeTextElement("PersonalSafety", QString::number(details.personalSafety));
	writer.writeTextElement("TimeFormat", QString::number(details.timeFormat));

	writer.writeStartElement("Light");
	writer.writeAttribute("enabled", QString::number(details.lightEnabled));
	writer.writeCharacters(QString::number(details.light));
	writer.writeEndElement();

	writer.writeStartElement("AlarmTime");
	writer.writeAttribute("enabled", QString::number(details.alarmTimeEnabled));
	writer.writeCharacters(QString::number(details.alarmTime));
	writer.writeEndElement();

	writer.writeStartElement("AlarmDepth");
	writer.writeAttribute("enabled", QString::number(details.alarmDepthEnabled));
	writer.writeCharacters(QString::number(details.alarmDepth.mm));
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

bool ConfigureDiveComputer::restoreXMLBackup(const QString &fileName, DeviceDetails &details)
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
				details.customText = keyString;

			if (settingName == "Gas1") {
				QStringList gasData = keyString.split(",");
				gas gas1;
				gas1.oxygen = gasData.at(0).toInt();
				gas1.helium = gasData.at(1).toInt();
				gas1.type = gasData.at(2).toInt();
				gas1.depth = gasData.at(3).toInt();
				details.gas1 = gas1;
			}

			if (settingName == "Gas2") {
				QStringList gasData = keyString.split(",");
				gas gas2;
				gas2.oxygen = gasData.at(0).toInt();
				gas2.helium = gasData.at(1).toInt();
				gas2.type = gasData.at(2).toInt();
				gas2.depth = gasData.at(3).toInt();
				details.gas2 = gas2;
			}

			if (settingName == "Gas3") {
				QStringList gasData = keyString.split(",");
				gas gas3;
				gas3.oxygen = gasData.at(0).toInt();
				gas3.helium = gasData.at(1).toInt();
				gas3.type = gasData.at(2).toInt();
				gas3.depth = gasData.at(3).toInt();
				details.gas3 = gas3;
			}

			if (settingName == "Gas4") {
				QStringList gasData = keyString.split(",");
				gas gas4;
				gas4.oxygen = gasData.at(0).toInt();
				gas4.helium = gasData.at(1).toInt();
				gas4.type = gasData.at(2).toInt();
				gas4.depth = gasData.at(3).toInt();
				details.gas4 = gas4;
			}

			if (settingName == "Gas5") {
				QStringList gasData = keyString.split(",");
				gas gas5;
				gas5.oxygen = gasData.at(0).toInt();
				gas5.helium = gasData.at(1).toInt();
				gas5.type = gasData.at(2).toInt();
				gas5.depth = gasData.at(3).toInt();
				details.gas5 = gas5;
			}

			if (settingName == "Dil1") {
				QStringList dilData = keyString.split(",");
				gas dil1;
				dil1.oxygen = dilData.at(0).toInt();
				dil1.helium = dilData.at(1).toInt();
				dil1.type = dilData.at(2).toInt();
				dil1.depth = dilData.at(3).toInt();
				details.dil1 = dil1;
			}

			if (settingName == "Dil2") {
				QStringList dilData = keyString.split(",");
				gas dil2;
				dil2.oxygen = dilData.at(0).toInt();
				dil2.helium = dilData.at(1).toInt();
				dil2.type = dilData.at(2).toInt();
				dil2.depth = dilData.at(3).toInt();
				details.dil1 = dil2;
			}

			if (settingName == "Dil3") {
				QStringList dilData = keyString.split(",");
				gas dil3;
				dil3.oxygen = dilData.at(0).toInt();
				dil3.helium = dilData.at(1).toInt();
				dil3.type = dilData.at(2).toInt();
				dil3.depth = dilData.at(3).toInt();
				details.dil3 = dil3;
			}

			if (settingName == "Dil4") {
				QStringList dilData = keyString.split(",");
				gas dil4;
				dil4.oxygen = dilData.at(0).toInt();
				dil4.helium = dilData.at(1).toInt();
				dil4.type = dilData.at(2).toInt();
				dil4.depth = dilData.at(3).toInt();
				details.dil4 = dil4;
			}

			if (settingName == "Dil5") {
				QStringList dilData = keyString.split(",");
				gas dil5;
				dil5.oxygen = dilData.at(0).toInt();
				dil5.helium = dilData.at(1).toInt();
				dil5.type = dilData.at(2).toInt();
				dil5.depth = dilData.at(3).toInt();
				details.dil5 = dil5;
			}

			if (settingName == "SetPoint1") {
				QStringList spData = keyString.split(",");
				setpoint sp1;
				sp1.sp = spData.at(0).toInt();
				sp1.depth = spData.at(1).toInt();
				details.sp1 = sp1;
			}

			if (settingName == "SetPoint2") {
				QStringList spData = keyString.split(",");
				setpoint sp2;
				sp2.sp = spData.at(0).toInt();
				sp2.depth = spData.at(1).toInt();
				details.sp2 = sp2;
			}

			if (settingName == "SetPoint3") {
				QStringList spData = keyString.split(",");
				setpoint sp3;
				sp3.sp = spData.at(0).toInt();
				sp3.depth = spData.at(1).toInt();
				details.sp3 = sp3;
			}

			if (settingName == "SetPoint4") {
				QStringList spData = keyString.split(",");
				setpoint sp4;
				sp4.sp = spData.at(0).toInt();
				sp4.depth = spData.at(1).toInt();
				details.sp4 = sp4;
			}

			if (settingName == "SetPoint5") {
				QStringList spData = keyString.split(",");
				setpoint sp5;
				sp5.sp = spData.at(0).toInt();
				sp5.depth = spData.at(1).toInt();
				details.sp5 = sp5;
			}

			if (settingName == "Saturation")
				details.saturation = keyString.toInt();

			if (settingName == "Desaturation")
				details.desaturation = keyString.toInt();

			if (settingName == "DiveMode")
				details.diveMode = keyString.toInt();

			if (settingName == "LastDeco")
				details.lastDeco = keyString.toInt();

			if (settingName == "Brightness")
				details.brightness = keyString.toInt();

			if (settingName == "Units")
				details.units = keyString.toInt();

			if (settingName == "SamplingRate")
				details.samplingRate = keyString.toInt();

			if (settingName == "Salinity")
				details.salinity = keyString.toInt();

			if (settingName == "DiveModeColour")
				details.diveModeColor = keyString.toInt();

			if (settingName == "Language")
				details.language = keyString.toInt();

			if (settingName == "DateFormat")
				details.dateFormat = keyString.toInt();

			if (settingName == "CompassGain")
				details.compassGain = keyString.toInt();

			if (settingName == "SafetyStop")
				details.safetyStop = keyString.toInt();

			if (settingName == "GfHigh")
				details.gfHigh = keyString.toInt();

			if (settingName == "GfLow")
				details.gfLow = keyString.toInt();

			if (settingName == "PressureSensorOffset")
				details.pressureSensorOffset = keyString.toInt();

			if (settingName == "PpO2Min")
				details.ppO2Min = keyString.toInt();

			if (settingName == "PpO2Max")
				details.ppO2Max = keyString.toInt();

			if (settingName == "FutureTTS")
				details.futureTTS = keyString.toInt();

			if (settingName == "CcrMode")
				details.ccrMode = keyString.toInt();

			if (settingName == "DecoType")
				details.decoType = keyString.toInt();

			if (settingName == "VPMConservatism")
				details.vpmConservatism = keyString.toInt();

			if (settingName == "AGFSelectable")
				details.aGFSelectable = keyString.toInt();

			if (settingName == "AGFHigh")
				details.aGFHigh = keyString.toInt();

			if (settingName == "AGFLow")
				details.aGFLow = keyString.toInt();

			if (settingName == "CalibrationGas")
				details.calibrationGas = keyString.toInt();

			if (settingName == "FlipScreen")
				details.flipScreen = keyString.toInt();

			if (settingName == "SetPointFallback")
				details.setPointFallback = keyString.toInt();

			if (settingName == "LeftButtonSensitivity")
				details.leftButtonSensitivity = keyString.toInt();

			if (settingName == "RightButtonSensitivity")
				details.rightButtonSensitivity = keyString.toInt();

			if (settingName == "ButtonSensitivity")
				details.buttonSensitivity = keyString.toInt();

			if (settingName == "ButtonBalance")
				details.buttonBalance = keyString.toInt();

			if (settingName == "BottomGasConsumption")
				details.bottomGasConsumption = keyString.toInt();

			if (settingName == "DecoGasConsumption")
				details.decoGasConsumption = keyString.toInt();

			if (settingName == "TravelGasConsumption")
				details.travelGasConsumption = keyString.toInt();

			if (settingName == "ModWarning")
				details.modWarning = keyString.toInt();

			if (settingName == "DynamicAscendRate")
				details.dynamicAscendRate = keyString.toInt();

			if (settingName == "SafetyStopEndDepth")
				details.safetyStopEndDepth = keyString.toInt();

			if (settingName == "SafetyStopResetDepth")
				details.safetyStopResetDepth = keyString.toInt();

			if (settingName == "GraphicalSpeedIndicator")
				details.graphicalSpeedIndicator = keyString.toInt();

			if (settingName == "AlwaysShowppO2")
				details.alwaysShowppO2 = keyString.toInt();

			if (settingName == "Altitude")
				details.altitude = keyString.toInt();

			if (settingName == "PersonalSafety")
				details.personalSafety = keyString.toInt();

			if (settingName == "TimeFormat")
				details.timeFormat = keyString.toInt();

			if (settingName == "Light") {
				if (attributes.hasAttribute("enabled"))
					details.lightEnabled = attributes.value("enabled").toString().toInt();
				details.light = keyString.toInt();
			}

			if (settingName == "AlarmDepth") {
				if (attributes.hasAttribute("enabled"))
					details.alarmDepthEnabled = attributes.value("enabled").toString().toInt();
				details.alarmDepth.mm = keyString.toInt();
			}

			if (settingName == "AlarmTime") {
				if (attributes.hasAttribute("enabled"))
					details.alarmTimeEnabled = attributes.value("enabled").toString().toInt();
				details.alarmTime = keyString.toInt();
			}
		}
		reader.readNext();
	}

	return true;
}

void ConfigureDiveComputer::startFirmwareUpdate(const QString &fileName, device_data_t *data, bool forceUpdate)
{
	if (currentState == READING || currentState == WRITING) {
		emit error(tr("Wait for the settings operation to finish before updating firmware."));
		return;
	}
	setState(FWUPDATE);
	if (firmwareThread)
		firmwareThread->deleteLater();

	firmwareThread = new FirmwareUpdateThread(this, data, fileName, forceUpdate);
	connectThreadSignals(firmwareThread);

	firmwareThread->start();
}

void ConfigureDiveComputer::resetSettings(device_data_t *data)
{
	if (currentState == READING || currentState == WRITING) {
		emit error(tr("Wait for the settings operation to finish before resetting."));
		return;
	}
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
	ConfigureDiveComputer::states oldState = currentState;
	currentState = newState;

	emit stateChanged(oldState, currentState);
}

void ConfigureDiveComputer::setError(QString err)
{
	// A close failure can follow a write/read-back error in the same worker.
	// Keep the earlier device outcome visible rather than replacing it with the
	// transport outcome.
	if (currentState == WRITING && !lastError.isEmpty())
		err = lastError + QStringLiteral("\n") + err;
	lastError = err;
	emit error(std::move(err));
}

void ConfigureDiveComputer::threadFinished()
{
	const bool success = lastError.isEmpty();
	const bool wasOstc4Write = ostc4Writing;
	if (ostc4Writing) {
		ostc4Snapshot.valid = false;
		ostc4Writing = false;
	}
	setState(success ? DONE : ERRORED);

	if (success && !wasOstc4Write &&
	    !(sender() == readThread && (ostc4Snapshot.identity.model == 0x43 || ostc4Snapshot.identity.model == 0x44)))
		emit message(tr("The operation completed successfully"));
}

QString ConfigureDiveComputer::openTransport(device_data_t *data)
{
	FILE *fp = NULL;
	dc_status_t rc;

	if (data->libdc_log && !logfile_name.empty())
		fp = subsurface_fopen(logfile_name.c_str(), "w");

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
	} else {
		dc_context_set_loglevel(data->context, get_libdivecomputer_loglevel());
	}

	rc = divecomputer_device_open(data);

	if (rc != DC_STATUS_SUCCESS) {
		report_error("%s", errmsg(rc));
	} else {
		rc = dc_device_open(&data->device, data->context, data->descriptor, data->iostream);
	}

	if (rc != DC_STATUS_SUCCESS) {
		return tr("Could not a establish connection to the dive computer.");
	}

	return NULL;
}

QString ConfigureDiveComputer::dc_open(device_data_t *data)
{
	if (currentState == READING || currentState == WRITING)
		return tr("Wait for the settings operation to finish before connecting.");
	// An OSTC3-family settings read owns its transport in the worker. Connecting
	// here would immediately close it and connect a second time on Retrieve.
	if (data->descriptor && dc_descriptor_get_type(data->descriptor) == DC_FAMILY_HW_OSTC3) {
		setState(OPEN);
		return {};
	}
	QString error = openTransport(data);
	if (error.isEmpty())
		setState(OPEN);
	return error;
}

QString ConfigureDiveComputer::openForOtherOperation(device_data_t *data)
{
	if (currentState == READING || currentState == WRITING)
		return tr("Wait for the settings operation to finish before connecting.");
	stopReadSession();
	if (data->device)
		return {};
	const QString error = openTransport(data);
	if (error.isEmpty())
		setState(OPEN);
	else
		closeTransport(data);
	return error;
}

dc_status_t ConfigureDiveComputer::closeTransport(device_data_t *data)
{
	dc_status_t status = DC_STATUS_SUCCESS;
	if (data->device)
		status = dc_device_close(data->device);
	data->device = NULL;
	// Close the iostream before freeing the context: dc_serial_close (and other iostream
	// close implementations) may call SYSERROR(abstract->context, ...) on error paths
	// (e.g. tcsetattr/ioctl failing on a disconnected RFCOMM fd). Freeing the context
	// first causes a use-after-free in those error paths.
	const dc_status_t streamStatus = dc_iostream_close(data->iostream);
	if (status == DC_STATUS_SUCCESS)
		status = streamStatus;
	data->iostream = NULL;
	if (data->context)
		dc_context_free(data->context);
	data->context = NULL;

	if (data->libdc_logfile)
		fclose(data->libdc_logfile);
	data->libdc_logfile = nullptr;
	return status;
}

void ConfigureDiveComputer::dc_close(device_data_t *data)
{
	stopReadSession();
	ostc4Snapshot.valid = false;
	closeTransport(data);
	setState(INITIAL);
}
