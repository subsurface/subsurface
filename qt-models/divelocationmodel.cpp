// SPDX-License-Identifier: GPL-2.0
#include "core/units.h"
#include "qt-models/divelocationmodel.h"
#include "core/subsurface-qt/DiveListNotifier.h"
#include "core/qthelper.h"
#include "core/divesite.h"
#include "core/metrics.h"
#include "cleanertablemodel.h" // for trashIcon();
#include <QDebug>
#include <QLineEdit>
#include <QIcon>
#include <core/gettextfromc.h>

static bool dive_site_less_than(dive_site *a, dive_site *b)
{
	return QString(a->name) < QString(b->name);
}

LocationInformationModel *LocationInformationModel::instance()
{
	static LocationInformationModel *self = new LocationInformationModel();
	return self;
}

LocationInformationModel::LocationInformationModel(QObject *obj) : QAbstractTableModel(obj)
{
	connect(&diveListNotifier, &DiveListNotifier::diveSiteDiveCountChanged, this, &LocationInformationModel::diveSiteDiveCountChanged);
}

int LocationInformationModel::columnCount(const QModelIndex &) const
{
	return COLUMNS;
}

int LocationInformationModel::rowCount(const QModelIndex &) const
{
	return dive_site_table.nr;
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

QVariant LocationInformationModel::getDiveSiteData(const struct dive_site *ds, int column, int role)
{
	if (!ds)
		return QVariant();

	switch(role) {
	case Qt::EditRole:
	case Qt::DisplayRole:
		switch(column) {
		case DIVESITE: return QVariant::fromValue<dive_site *>((dive_site *)ds); // Not nice: casting away const
		case NAME: return ds->name;
		case NUM_DIVES: return ds->dives.nr;
		case LATITUDE: return ds->location.lat.udeg;
		case LONGITUDE: return ds->location.lon.udeg;
		case COORDS: return "TODO";
		case DESCRIPTION: return ds->description;
		case NOTES: return ds->name;
		case TAXONOMY_1: return "TODO";
		case TAXONOMY_2: return "TODO";
		case TAXONOMY_3: return "TODO";
		}
	break;
	case Qt::ToolTipRole:
		switch(column) {
		case REMOVE: return tr("Clicking here will remove this divesite.");
		}
	break;
	case Qt::DecorationRole:
		switch(column) {
#ifndef SUBSURFACE_MOBILE
		case REMOVE: return trashIcon();
#endif
		case NAME: return dive_site_has_gps_location(ds) ? QIcon(":geotag-icon") : QVariant();
		}
	break;
	case DIVESITE_ROLE:
		return QVariant::fromValue<dive_site *>((dive_site *)ds); // Not nice: casting away const
	}
	return QVariant();
}

QVariant LocationInformationModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	struct dive_site *ds = get_dive_site(index.row(), &dive_site_table);
	return getDiveSiteData(ds, index.column(), role);
}

void LocationInformationModel::update()
{
	beginResetModel();
	qSort(dive_site_table.dive_sites, dive_site_table.dive_sites + dive_site_table.nr, dive_site_less_than);
	locationNames.clear();
	for (int i = 0; i < dive_site_table.nr; i++)
		locationNames << QString(dive_site_table.dive_sites[i]->name);
	endResetModel();
}

QStringList LocationInformationModel::allSiteNames() const
{
	return locationNames;
}

bool LocationInformationModel::removeRows(int row, int, const QModelIndex&)
{
	if(row >= rowCount())
		return false;

	beginRemoveRows(QModelIndex(), row, row);
	struct dive_site *ds = get_dive_site(row, &dive_site_table);
	if (ds)
		delete_dive_site(ds, &dive_site_table);
	endRemoveRows();
	return true;
}

void LocationInformationModel::diveSiteDiveCountChanged(dive_site *ds)
{
	int idx = get_divesite_idx(ds, &dive_site_table);
	if (idx >= 0)
		dataChanged(createIndex(idx, NUM_DIVES), createIndex(idx, NUM_DIVES));
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
	struct dive_site *ds = sourceModel()->index(sourceRow, LocationInformationModel::DIVESITE, parent).data().value<dive_site *>();
	if (ds == ignoreDs || ds == RECENTLY_ADDED_DIVESITE)
		return false;

	return ds && same_location(&ds->location, &location);
}

GPSLocationInformationModel::GPSLocationInformationModel(QObject *parent) : QSortFilterProxyModel(parent),
	ignoreDs(nullptr),
	location({{0},{0}})
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
