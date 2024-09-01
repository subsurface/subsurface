// SPDX-License-Identifier: GPL-2.0
#include "core/units.h"
#include "qt-models/divelocationmodel.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/errorhelper.h"
#include "core/divesite.h"
#include "core/divelog.h"
#include "core/metrics.h"
#ifndef SUBSURFACE_MOBILE
#include "cleanertablemodel.h" // for trashIcon() and editIcon()
#include "commands/command.h"
#endif
#include <QLineEdit>
#include <QIcon>
#include <core/gettextfromc.h>

LocationInformationModel *LocationInformationModel::instance()
{
	static LocationInformationModel *self = new LocationInformationModel();
	return self;
}

LocationInformationModel::LocationInformationModel(QObject *obj) : QAbstractTableModel(obj)
{
	connect(&diveListNotifier, &DiveListNotifier::diveSiteDiveCountChanged, this, &LocationInformationModel::diveSiteDiveCountChanged);
	connect(&diveListNotifier, &DiveListNotifier::diveSiteAdded, this, &LocationInformationModel::diveSiteAdded);
	connect(&diveListNotifier, &DiveListNotifier::diveSiteDeleted, this, &LocationInformationModel::diveSiteDeleted);
	connect(&diveListNotifier, &DiveListNotifier::diveSiteChanged, this, &LocationInformationModel::diveSiteChanged);
	connect(&diveListNotifier, &DiveListNotifier::diveSiteDivesChanged, this, &LocationInformationModel::diveSiteDivesChanged);
	connect(&diveListNotifier, &DiveListNotifier::dataReset, this, &LocationInformationModel::update);
}

int LocationInformationModel::columnCount(const QModelIndex &) const
{
	return COLUMNS;
}

int LocationInformationModel::rowCount(const QModelIndex &) const
{
	return (int)divelog.sites.size();
}

QVariant LocationInformationModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Vertical)
		return QVariant();

	switch (role) {
	case Qt::TextAlignmentRole:
		return int(Qt::AlignLeft | Qt::AlignVCenter);
	case Qt::FontRole:
		return defaultModelFont();
	case Qt::InitialSortOrderRole:
		// By default, sort number of dives descending, everything else ascending.
		return section == NUM_DIVES ? Qt::DescendingOrder : Qt::AscendingOrder;
	case Qt::DisplayRole:
	case Qt::ToolTipRole:
		switch (section) {
		case NAME:
			return tr("Name");
		case DESCRIPTION:
			return tr("Description");
		case NUM_DIVES:
			return tr("# of dives");
		}
		break;
	}

	return QVariant();
}

Qt::ItemFlags LocationInformationModel::flags(const QModelIndex &index) const
{
	switch (index.column()) {
	case REMOVE:
		return Qt::ItemIsEnabled;
	case NAME:
	case DESCRIPTION:
		return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
	}
	return QAbstractItemModel::flags(index);
}

QVariant LocationInformationModel::getDiveSiteData(const struct dive_site &ds, int column, int role)
{
	switch(role) {
	case Qt::EditRole:
	case Qt::DisplayRole:
		switch(column) {
		case DIVESITE: return QVariant::fromValue<dive_site *>((dive_site *)&ds); // Not nice: casting away const
		case NAME: return QString::fromStdString(ds.name);
		case NUM_DIVES: return static_cast<int>(ds.dives.size());
		case LOCATION: return "TODO";
		case DESCRIPTION: return QString::fromStdString(ds.description);
		case NOTES: return QString::fromStdString(ds.notes);
		case TAXONOMY: return "TODO";
		}
	break;
	case Qt::ToolTipRole:
		switch(column) {
		case EDIT: return tr("Click here to edit the divesite.");
		case REMOVE: return tr("Clicking here will remove this divesite.");
		}
	break;
	case Qt::DecorationRole:
		switch(column) {
#ifndef SUBSURFACE_MOBILE
		case EDIT: return editIcon();
		case REMOVE: return trashIcon();
#endif
		case NAME: return ds.has_gps_location() ? QIcon(":geotag-icon") : QVariant();
		}
	break;
	case DIVESITE_ROLE:
		return QVariant::fromValue<dive_site *>((dive_site *)&ds); // Not nice: casting away const
	}
	return QVariant();
}

QVariant LocationInformationModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() >= (int)divelog.sites.size())
		return QVariant();

	const auto &ds = (divelog.sites)[index.row()].get();
	return getDiveSiteData(*ds, index.column(), role);
}

void LocationInformationModel::update()
{
	beginResetModel();
	endResetModel();
}

void LocationInformationModel::diveSiteDiveCountChanged(dive_site *ds)
{
	size_t idx = divelog.sites.get_idx(ds);
	if (idx != std::string::npos)
		dataChanged(createIndex(idx, NUM_DIVES), createIndex(idx, NUM_DIVES));
}

void LocationInformationModel::diveSiteAdded(struct dive_site *, int idx)
{
	if (idx < 0)
		return;
	beginInsertRows(QModelIndex(), idx, idx);
	// Row has already been added by Undo-Command.
	endInsertRows();
}

void LocationInformationModel::diveSiteDeleted(struct dive_site *, int idx)
{
	if (idx < 0)
		return;
	beginRemoveRows(QModelIndex(), idx, idx);
	// Row has already been added by Undo-Command.
	endRemoveRows();
}

void LocationInformationModel::diveSiteChanged(struct dive_site *ds, int field)
{
	size_t idx = divelog.sites.get_idx(ds);
	if (idx == std::string::npos)
		return;
	dataChanged(createIndex(idx, field), createIndex(idx, field));
}

void LocationInformationModel::diveSiteDivesChanged(struct dive_site *ds)
{
	size_t idx = divelog.sites.get_idx(ds);
	if (idx == std::string::npos)
		return;
	dataChanged(createIndex(idx, NUM_DIVES), createIndex(idx, NUM_DIVES));
}

bool DiveSiteSortedModel::filterAcceptsRow(int sourceRow, const QModelIndex &source_parent) const
{
	if (fullText.isEmpty())
		return true;

	if (sourceRow < 0 || sourceRow > (int)divelog.sites.size())
		return false;
	const auto &ds = (divelog.sites)[sourceRow];
	QString text = QString::fromStdString(ds->name + ds->description + ds->notes);
	return text.contains(fullText, Qt::CaseInsensitive);
}

bool DiveSiteSortedModel::lessThan(const QModelIndex &i1, const QModelIndex &i2) const
{
	// The source indices correspond to indices in the global dive site table.
	// Let's access them directly without going via the source model.
	// Kind of dirty, but less effort.

	// Be careful to respect proper ordering when sites are invalid.
	bool valid1 = i1.row() >= 0 && i1.row() < (int)divelog.sites.size();
	bool valid2 = i2.row() >= 0 && i2.row() < (int)divelog.sites.size();
	if (!valid1 || !valid2)
		return valid1 < valid2;

	const auto &ds1 = (divelog.sites)[i1.row()];
	const auto &ds2 = (divelog.sites)[i2.row()];
	switch (i1.column()) {
	case LocationInformationModel::NAME:
	default:
		return QString::localeAwareCompare(QString::fromStdString(ds1->name), QString::fromStdString(ds2->name)) < 0; // TODO: avoid copy
	case LocationInformationModel::DESCRIPTION: {
		int cmp = QString::localeAwareCompare(QString::fromStdString(ds1->description), QString::fromStdString(ds2->description)); // TODO: avoid copy
		return cmp != 0 ? cmp < 0 :
		       QString::localeAwareCompare(QString::fromStdString(ds1->name), QString::fromStdString(ds2->name)) < 0; // TODO: avoid copy
	}
	case LocationInformationModel::NUM_DIVES: {
		int cmp = static_cast<int>(ds1->dives.size()) - static_cast<int>(ds2->dives.size());
		// Since by default nr dives is descending, invert sort direction of names, such that
		// the names are listed as ascending.
		return cmp != 0 ? cmp < 0 :
		       QString::localeAwareCompare(QString::fromStdString(ds1->name), QString::fromStdString(ds2->name)) < 0; // TODO: avoid copy
	}
	}
}

DiveSiteSortedModel::DiveSiteSortedModel(QObject *parent) : QSortFilterProxyModel(parent)
{
	setSourceModel(LocationInformationModel::instance());
}

QStringList DiveSiteSortedModel::allSiteNames() const
{
	QStringList locationNames;
	int num = rowCount();
	for (int i = 0; i < num; i++) {
		int idx = mapToSource(index(i, 0)).row();
		// This shouldn't happen, but if model and core get out of sync,
		// (more precisely: the core has more sites than the model is aware of),
		// we might get an invalid index.
		if (idx < 0 || idx > (int)divelog.sites.size()) {
			report_info("DiveSiteSortedModel::allSiteNames(): invalid index");
			continue;
		}
		locationNames << QString::fromStdString((divelog.sites)[idx]->name);
	}
	return locationNames;
}

struct dive_site *DiveSiteSortedModel::getDiveSite(const QModelIndex &idx_source)
{
	auto idx = mapToSource(idx_source).row();
	return idx >= 0 && idx < (int)divelog.sites.size() ? (divelog.sites)[idx].get() : NULL;
}

#ifndef SUBSURFACE_MOBILE
bool DiveSiteSortedModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	struct dive_site *ds = getDiveSite(index);
	if (!ds || value.isNull())
		return false;
	switch (index.column()) {
	case LocationInformationModel::NAME:
		Command::editDiveSiteName(ds, value.toString());
		return true;
	case LocationInformationModel::DESCRIPTION:
		Command::editDiveSiteDescription(ds, value.toString());
		return true;
	default:
		return false;
	}
}

#endif // SUBSURFACE_MOBILE

void DiveSiteSortedModel::setFilter(const QString &text)
{
	fullText = text.trimmed();
	invalidateFilter();
}

GeoReferencingOptionsModel *GeoReferencingOptionsModel::instance()
{
	static GeoReferencingOptionsModel *self = new GeoReferencingOptionsModel();
	return self;
}

GeoReferencingOptionsModel::GeoReferencingOptionsModel(QObject *parent) : QStringListModel(parent)
{
	QStringList list;
	int i;
	for (i = 0; i < TC_NR_CATEGORIES; i++)
		list << gettextFromC::tr(taxonomy_category_names[i]);
	setStringList(list);
}

bool GPSLocationInformationModel::filterAcceptsRow(int sourceRow, const QModelIndex &parent) const
{
	if (!has_location(&location))
		return false;

	struct dive_site *ds = sourceModel()->index(sourceRow, LocationInformationModel::DIVESITE, parent).data().value<dive_site *>();
	if (!ds || ds == ignoreDs || ds == RECENTLY_ADDED_DIVESITE || !has_location(&ds->location))
		return false;

	return distance <= 0 ? ds->location == location
			     : (int64_t)get_distance(ds->location, location) * 1000 <= distance; // We need 64 bit to represent distances in mm
}

GPSLocationInformationModel::GPSLocationInformationModel(QObject *parent) : QSortFilterProxyModel(parent),
	ignoreDs(nullptr),
	distance(0)
{
	setSourceModel(LocationInformationModel::instance());
}

void GPSLocationInformationModel::set(const struct dive_site *ignoreDsIn, const location_t &locationIn)
{
	ignoreDs = ignoreDsIn;
	location = locationIn;
	invalidate();
}

void GPSLocationInformationModel::setCoordinates(const location_t &locationIn)
{
	location = locationIn;
	invalidate();
}

void GPSLocationInformationModel::setDistance(int64_t dist)
{
	distance = dist;
	invalidate();
}
