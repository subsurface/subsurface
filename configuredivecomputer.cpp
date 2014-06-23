#include "configuredivecomputer.h"
#include "libdivecomputer/hw.h"
#include <QDebug>
#include <QFile>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/tree.h>
#include <libxslt/transform.h>
#include <QStringList>

ConfigureDiveComputer::ConfigureDiveComputer(QObject *parent) :
	QObject(parent),
	readThread(0),
	writeThread(0)
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

bool ConfigureDiveComputer::saveXMLBackup(QString fileName, DeviceDetails *details, device_data_t *data, QString errorText)
{
	QString xml = "";
	QString vendor = data->vendor;
	QString product = data->product;
	xml += "<DiveComputerSettingsBackup>";
	xml += "\n<DiveComputer>";
	xml += addSettingToXML("Vendor", vendor);
	xml += addSettingToXML("Product", product);
	xml += "\n</DiveComputer>";
	xml += "\n<Settings>";
	xml += addSettingToXML("CustomText", details->customText());
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
	xml += addSettingToXML("Gas1", gas1);
	xml += addSettingToXML("Gas2", gas2);
	xml += addSettingToXML("Gas3", gas3);
	xml += addSettingToXML("Gas4", gas4);
	xml += addSettingToXML("Gas5", gas5);
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
	xml += addSettingToXML("Dil1", dil1);
	xml += addSettingToXML("Dil2", dil2);
	xml += addSettingToXML("Dil3", dil3);
	xml += addSettingToXML("Dil4", dil4);
	xml += addSettingToXML("Dil5", dil5);
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
	xml += addSettingToXML("SetPoint1", sp1);
	xml += addSettingToXML("SetPoint2", sp2);
	xml += addSettingToXML("SetPoint3", sp3);
	xml += addSettingToXML("SetPoint4", sp4);
	xml += addSettingToXML("SetPoint5", sp5);

	//Other Settings
	xml += addSettingToXML("DiveMode", details->diveMode());
	xml += addSettingToXML("Saturation", details->saturation());
	xml += addSettingToXML("Desaturation", details->desaturation());
	xml += addSettingToXML("LastDeco", details->lastDeco());
	xml += addSettingToXML("Brightness", details->brightness());
	xml += addSettingToXML("Units", details->units());
	xml += addSettingToXML("SamplingRate", details->samplingRate());
	xml += addSettingToXML("Salinity", details->salinity());
	xml += addSettingToXML("DiveModeColor", details->diveModeColor());
	xml += addSettingToXML("Language", details->language());
	xml += addSettingToXML("DateFormat", details->dateFormat());
	xml += addSettingToXML("CompassGain", details->compassGain());
	xml += "\n</Settings>";
	xml += "\n</DiveComputerSettingsBackup>";
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly)) {
		errorText = tr("Could not save the backup file %1. Error Message: %2")
				.arg(fileName, file.errorString());
		return false;
	}
	//file open successful. write data and save.
	QTextStream out(&file);
	out << xml;

	file.close();
	return true;
}

bool ConfigureDiveComputer::restoreXMLBackup(QString fileName, DeviceDetails *details, QString errorText)
{
	xmlDocPtr doc;
	xmlNodePtr node;
	xmlChar *key;

	doc = xmlParseFile(fileName.toUtf8().data());

	if (doc == NULL) {
		errorText = tr("Could not read the backup file.");
		return false;
	}

	node = xmlDocGetRootElement(doc);
	if (node == NULL) {
		errorText = tr("The specified file is invalid.");
		xmlFreeDoc(doc);
		return false;
	}

	if (xmlStrcmp(node->name, (const xmlChar *) "DiveComputerSettingsBackup")) {
		errorText = tr("The specified file does not contain a valid backup.");
		xmlFreeDoc(doc);
		return false;
	}

	xmlNodePtr child = node->children;

	while (child != NULL) {
		QString nodeName = (char *)child->name;
		if (nodeName == "Settings") {
			xmlNodePtr settingNode = child->children;
			while (settingNode != NULL) {
				QString settingName = (char *)settingNode->name;
				key = xmlNodeListGetString(doc, settingNode->xmlChildrenNode, 1);
				QString keyString = (char *)key;
				if (settingName != "text") {
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
						details->setGas1(gas2);
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
				}

				settingNode = settingNode->next;
			}
		}
		child = child->next;
	}

	return true;
}

void ConfigureDiveComputer::startFirmwareUpdate(QString fileName, device_data_t *data, QString errorText)
{

}

void ConfigureDiveComputer::setState(ConfigureDiveComputer::states newState)
{
	currentState = newState;
	emit stateChanged(currentState);
}


QString ConfigureDiveComputer::addSettingToXML(QString settingName, QVariant value)
{
	return "\n<" + settingName + ">" + value.toString() + "</" + settingName + ">";
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
