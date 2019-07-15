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

#include <QLocale>

Qt::ItemFlags GasSelectionModel::flags(const QModelIndex&) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

GasSelectionModel *GasSelectionModel::instance()
{
	static GasSelectionModel self;
	return &self;
}

static QStringList getGasList()
{
	QStringList list;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = &displayed_dive.cylinder[i];
		if (cylinder_nodata(cyl))
			break;
		/* Check if we have the same gasmix two or more times
		 * If yes return more verbose string */
		int same_gas = same_gasmix_cylinder(cyl, i, &displayed_dive, true);
		if (same_gas == -1)
			list.push_back(get_gas_string(cyl->gasmix));
		else
			list.push_back(get_gas_string(cyl->gasmix) + QString(" (%1 %2 ").arg(GasSelectionModel::tr("cyl.")).arg(i + 1) +
				cyl->type.description + ")");
	}
	return list;
}

void GasSelectionModel::repopulate()
{
	setStringList(getGasList());
}

QVariant GasSelectionModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::FontRole) {
		return defaultModelFont();
	}
	return QStringListModel::data(index, role);
}
// Dive Type Model for the divetype combo box

Qt::ItemFlags DiveTypeSelectionModel::flags(const QModelIndex&) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

DiveTypeSelectionModel *DiveTypeSelectionModel::instance()
{
	static DiveTypeSelectionModel self;
	return &self;
}

void DiveTypeSelectionModel::repopulate()
{
	QStringList modes = QStringList();
	for (int i = 0; i < FREEDIVE; i++)
		modes.append(gettextFromC::tr(divemode_text_ui[i]));
	setStringList(modes);
}

QVariant DiveTypeSelectionModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::FontRole) {
		return defaultModelFont();
	}
	return QStringListModel::data(index, role);
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
	Q_FOREACH (const QString &s, d.entryList()) {
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
