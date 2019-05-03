// SPDX-License-Identifier: GPL-2.0
#include <QDebug>
#include "maplocationmodel.h"
#include "core/divesite.h"
#ifndef SUBSURFACE_MOBILE
#include "qt-models/filtermodels.h"
#endif

const char *MapLocation::PROPERTY_NAME_COORDINATE = "coordinate";
const char *MapLocation::PROPERTY_NAME_DIVESITE   = "divesite";
const char *MapLocation::PROPERTY_NAME_NAME       = "name";

#define MIN_DISTANCE_BETWEEN_DIVE_SITES_M 50.0

MapLocation::MapLocation() : m_ds(nullptr)
{
}

MapLocation::MapLocation(struct dive_site *ds, QGeoCoordinate coord, QString name) :
    m_ds(ds), m_coordinate(coord), m_name(name)
{
}

QVariant MapLocation::getRole(int role) const
{
	switch (role) {
	case Roles::RoleDivesite:
		return QVariant::fromValue((dive_site *)m_ds);
	case Roles::RoleCoordinate:
		return QVariant::fromValue(m_coordinate);
	case Roles::RoleName:
		return QVariant::fromValue(m_name);
	default:
		return QVariant();
	}
}

QGeoCoordinate MapLocation::coordinate()
{
	return m_coordinate;
}

void MapLocation::setCoordinate(QGeoCoordinate coord)
{
	m_coordinate = coord;
	emit coordinateChanged();
}

void MapLocation::setCoordinateNoEmit(QGeoCoordinate coord)
{
	m_coordinate = coord;
}

struct dive_site *MapLocation::divesite()
{
	return m_ds;
}

QVariant MapLocation::divesiteVariant()
{
	return QVariant::fromValue(m_ds);
}

MapLocationModel::MapLocationModel(QObject *parent) : QAbstractListModel(parent)
{
	m_roles[MapLocation::Roles::RoleDivesite] = MapLocation::PROPERTY_NAME_DIVESITE;
	m_roles[MapLocation::Roles::RoleCoordinate] = MapLocation::PROPERTY_NAME_COORDINATE;
	m_roles[MapLocation::Roles::RoleName] = MapLocation::PROPERTY_NAME_NAME;
}

MapLocationModel::~MapLocationModel()
{
	qDeleteAll(m_mapLocations);
}

QVariant MapLocationModel::data(const QModelIndex & index, int role) const
{
	if (index.row() < 0 || index.row() >= m_mapLocations.size())
		return QVariant();

	return m_mapLocations.at(index.row())->getRole(role);
}

QHash<int, QByteArray> MapLocationModel::roleNames() const
{
	return m_roles;
}

int MapLocationModel::rowCount(const QModelIndex&) const
{
	return m_mapLocations.size();
}

int MapLocationModel::count()
{
	return m_mapLocations.size();
}

MapLocation *MapLocationModel::get(int row)
{
	if (row < 0 || row >= m_mapLocations.size())
		return NULL;
	return m_mapLocations.at(row);
}

void MapLocationModel::add(MapLocation *location)
{
	beginInsertRows(QModelIndex(), m_mapLocations.size(), m_mapLocations.size());
	m_mapLocations.append(location);
	endInsertRows();
}

const QVector<dive_site *> &MapLocationModel::selectedDs() const
{
	return m_selectedDs;
}

void MapLocationModel::reload()
{
	int idx;
	struct dive *dive;

	beginResetModel();

	qDeleteAll(m_mapLocations);
	m_mapLocations.clear();
	m_selectedDs.clear();

	QMap<QString, MapLocation *> locationNameMap;
	MapLocation *location;
	QVector<struct dive_site *> locations;
	qreal latitude, longitude;

#ifdef SUBSURFACE_MOBILE
	bool diveSiteMode = false;
#else
	// In dive site mode (that is when either editing a dive site or on
	// the dive site tab), we want to show all dive sites, not only those
	// of the non-hidden dives. Moreover, the selected dive sites are those
	// that we filter for.
	bool diveSiteMode = MultiFilterSortModel::instance()->diveSiteMode();
	if (diveSiteMode)
		m_selectedDs = MultiFilterSortModel::instance()->filteredDiveSites();
#endif
	for_each_dive(idx, dive) {
		// Don't show dive sites of hidden dives, unless this is the currently
		// displayed (edited) dive or we're in dive site edit mode.
		if (!diveSiteMode && dive->hidden_by_filter && dive != current_dive)
			continue;
		struct dive_site *ds = get_dive_site_for_dive(dive);
		if (!dive_site_has_gps_location(ds))
			continue;
		if (!diveSiteMode && dive->selected && !m_selectedDs.contains(ds))
			m_selectedDs.append(ds);
		if (locations.contains(ds))
			continue;
		latitude = ds->location.lat.udeg * 0.000001;
		longitude = ds->location.lon.udeg * 0.000001;
		QGeoCoordinate dsCoord(latitude, longitude);
		QString name(ds->name);
		// don't add dive locations with the same name, unless they are
		// at least MIN_DISTANCE_BETWEEN_DIVE_SITES_M apart
		if (locationNameMap.contains(name)) {
			MapLocation *existingLocation = locationNameMap[name];
			QGeoCoordinate coord = existingLocation->coordinate();
			if (dsCoord.distanceTo(coord) < MIN_DISTANCE_BETWEEN_DIVE_SITES_M)
				continue;
		}
		location = new MapLocation(ds, dsCoord, name);
		m_mapLocations.append(location);
		locations.append(ds);
		locationNameMap[name] = location;
	}

	endResetModel();
}

void MapLocationModel::setSelected(struct dive_site *ds, bool fromClick)
{
	m_selectedDs.clear();
	m_selectedDs.append(ds);
	if (fromClick)
		emit selectedLocationChanged(getMapLocation(ds));
}

bool MapLocationModel::isSelected(const QVariant &dsVariant) const
{
	dive_site *ds = dsVariant.value<dive_site *>();
	return ds && m_selectedDs.contains(ds);
}

MapLocation *MapLocationModel::getMapLocation(const struct dive_site *ds)
{
	MapLocation *location;
	foreach(location, m_mapLocations) {
		if (ds == location->divesite())
			return location;
	}
	return NULL;
}

void MapLocationModel::updateMapLocationCoordinates(const struct dive_site *ds, QGeoCoordinate coord)
{
	MapLocation *location;
	int row = 0;
	foreach(location, m_mapLocations) {
		if (ds == location->divesite()) {
			location->setCoordinateNoEmit(coord);
			emit dataChanged(createIndex(row, 0), createIndex(row, 0));
			return;
		}
		row++;
	}
	// should not happen, as this should be called only when editing an existing marker
	qWarning() << "MapLocationModel::updateMapLocationCoordinates(): cannot find MapLocation for uuid:" << (ds ? ds->uuid : 0);
}
