// SPDX-License-Identifier: GPL-2.0
#include "core/units.h"
#include "qt-models/divelocationmodel.h"
#include "core/qthelper.h"
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
}

int LocationInformationModel::columnCount(const QModelIndex&) const
{
	return COLUMNS;
}

int LocationInformationModel::rowCount(const QModelIndex&) const
{
	return dive_site_table.nr;
}

QVariant LocationInformationModel::getDiveSiteData(const struct dive_site *ds, int column, int role)
{
	if (!ds)
		return QVariant();

	switch(role) {
	case Qt::EditRole:
	case Qt::DisplayRole :
		switch(column) {
		case UUID: return ds->uuid;
		case NAME: return ds->name;
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
	case Qt::DecorationRole : {
		if (dive_site_has_gps_location(ds))
			return QIcon(":geotag-icon");
		else
			return QVariant();
	}
	case DIVESITE_ROLE:
		return QVariant::fromValue<void *>((void *)ds); // Not nice: casting away const
	}
	return QVariant();
}

QVariant LocationInformationModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	struct dive_site *ds = get_dive_site(index.row());
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
	struct dive_site *ds = get_dive_site(row);
	if (ds)
		delete_dive_site(ds);
	endRemoveRows();
	return true;
}

GeoReferencingOptionsModel *GeoReferencingOptionsModel::instance() {
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
	uint32_t uuid = sourceModel()->index(sourceRow, LocationInformationModel::UUID, parent).data().toUInt();
	if (uuid == ignoreUuid || uuid == RECENTLY_ADDED_DIVESITE)
		return false;
	struct dive_site *ds = get_dive_site_by_uuid(uuid);

	return ds && same_location(&ds->location, &location);
}

GPSLocationInformationModel::GPSLocationInformationModel(QObject *parent) : QSortFilterProxyModel(parent),
	ignoreUuid(0),
	location({{0},{0}})
{
	setSourceModel(LocationInformationModel::instance());
}

void GPSLocationInformationModel::set(uint32_t ignoreUuidIn, const location_t &locationIn)
{
	ignoreUuid = ignoreUuidIn;
	location = locationIn;
	invalidate();
}

void GPSLocationInformationModel::setCoordinates(const location_t &locationIn)
{
	location = locationIn;
	invalidate();
}
