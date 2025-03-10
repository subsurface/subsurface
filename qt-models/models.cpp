// SPDX-License-Identifier: GPL-2.0
/*
 * models.cpp
 *
 * classes for the equipment models of Subsurface
 *
 */
#include "qt-models/models.h"
#include "core/qthelper.h"
#include "core/dive.h"
#include "core/gettextfromc.h"
#include "core/sample.h" // for NO_SENSOR

#include <QDir>
#include <QLocale>

GasSelectionModel::GasSelectionModel(const dive &d, int dcNr, QObject *parent)
	: QAbstractListModel(parent)
{
	gasNames = get_dive_gas_list(&d, dcNr, true);
}

QVariant GasSelectionModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	switch (role) {
	case Qt::FontRole:
		return defaultModelFont();
	case Qt::DisplayRole:
		return gasNames.at(index.row()).second;
	case Qt::UserRole:
		return gasNames.at(index.row()).first;
	}

	return QVariant();
}

int GasSelectionModel::rowCount(const QModelIndex&) const
{
	return gasNames.size();
}

// Dive Type Model for the divetype combo box

DiveTypeSelectionModel::DiveTypeSelectionModel(const dive &d, int dcNr, QObject *parent) : QAbstractListModel(parent)
{
	divemode_t mode = d.get_dc(dcNr)->divemode;
	for (int i = 0; i < FREEDIVE; i++) {
		switch (mode) {
		case OC:
		default:
			if (i != OC)
				continue;

			break;
		case CCR:
			if (i != OC && i != CCR)
				continue;

			break;
		case PSCR:
			if (i != OC && i != PSCR)
				continue;

			break;
		}


		diveTypes.push_back(std::pair(i, gettextFromC::tr(divemode_text_ui[i])));
	}
}

QVariant DiveTypeSelectionModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	switch (role) {
	case Qt::FontRole:
		return defaultModelFont();
	case Qt::DisplayRole:
		return diveTypes.at(index.row()).second;
	case Qt::UserRole:
		return diveTypes.at(index.row()).first;
	}

	return QVariant();
}

int DiveTypeSelectionModel::rowCount(const QModelIndex&) const
{
	return diveTypes.size();
}

SensorSelectionModel::SensorSelectionModel(const divecomputer &dc, QObject *parent)
	: QAbstractListModel(parent)
{
	sensorNames = get_tank_sensor_list(dc);
	sensorNames.insert(sensorNames.begin(), std::make_pair(NO_SENSOR, "<" + tr("none") + ">"));
}

QVariant SensorSelectionModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	switch (role) {
	case Qt::FontRole:
		return defaultModelFont();
	case Qt::DisplayRole:
		return sensorNames.at(index.row()).second;
	case Qt::UserRole:
		return sensorNames.at(index.row()).first;
	}

	return QVariant();
}

int SensorSelectionModel::rowCount(const QModelIndex&) const
{
	return sensorNames.size();
}

// Language Model, The Model to populate the list of possible Languages.

LanguageModel *LanguageModel::instance()
{
	static LanguageModel *self = new LanguageModel();
	QLocale l;
	return self;
}

LanguageModel::LanguageModel(QObject *parent) : QAbstractListModel(parent)
{
	QDir d(getSubsurfaceDataPath("translations"));
	for (const QString &s: d.entryList()) {
		if (s.startsWith("subsurface_") && s.endsWith(".qm")) {
			languages.push_back((s == "subsurface_source.qm") ? "English" : s);
		}
	}
}

QVariant LanguageModel::data(const QModelIndex &index, int role) const
{
	QLocale loc;
	QString currentString = languages.at(index.row());
	if (!index.isValid())
		return QVariant();
	switch (role) {
	case Qt::DisplayRole: {
		QLocale l(currentString.remove("subsurface_").remove(".qm"));
		return currentString == "English" ? currentString : QString("%1 (%2)").arg(l.languageToString(l.language())).arg(l.countryToString(l.country()));
	}
	case Qt::UserRole:
		return currentString == "English" ? "en_US" : currentString.remove("subsurface_").remove(".qm");
	}
	return QVariant();
}

int LanguageModel::rowCount(const QModelIndex&) const
{
	return languages.count();
}
