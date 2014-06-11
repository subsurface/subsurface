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
	connect (readThread, SIGNAL(finished()),
		 this, SLOT(readThreadFinished()), Qt::QueuedConnection);
	connect (readThread, SIGNAL(error(QString)), this, SLOT(setError(QString)));
	connect (readThread, SIGNAL(devicedetails(DeviceDetails*)), this,
		 SIGNAL(deviceDetailsChanged(DeviceDetails*)));

	readThread->start();
}

void ConfigureDiveComputer::saveDeviceDetails(DeviceDetails *details, device_data_t *data)
{
	setState(WRITING);

	if (writeThread)
		writeThread->deleteLater();

	writeThread = new WriteSettingsThread(this, data);
	connect (writeThread, SIGNAL(finished()),
		 this, SLOT(writeThreadFinished()), Qt::QueuedConnection);
	connect (writeThread, SIGNAL(error(QString)), this, SLOT(setError(QString)));

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
