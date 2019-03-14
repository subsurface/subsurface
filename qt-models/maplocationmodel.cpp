// SPDX-License-Identifier: GPL-2.0
#include <QDebug>
#include "maplocationmodel.h"
#include "core/divesite.h"

const char *MapLocation::PROPERTY_NAME_COORDINATE = "coordinate";
const char *MapLocation::PROPERTY_NAME_DIVESITE   = "divesite";
const char *MapLocation::PROPERTY_NAME_NAME       = "name";

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

MapLocationModel::MapLocationModel(QObject *parent) : QAbstractListModel(parent),
	m_selectedDs(nullptr)
{
	m_roles[MapLocation::Roles::RoleDivesite] = MapLocation::PROPERTY_NAME_DIVESITE;
	m_roles[MapLocation::Roles::RoleCoordinate] = MapLocation::PROPERTY_NAME_COORDINATE;
	m_roles[MapLocation::Roles::RoleName] = MapLocation::PROPERTY_NAME_NAME;
}

MapLocationModel::~MapLocationModel()
{
	clear();
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

void MapLocationModel::addList(QVector<MapLocation *> list)
{
	if (!list.size())
		return;
	beginInsertRows(QModelIndex(), m_mapLocations.size(), m_mapLocations.size() + list.size() - 1);
	m_mapLocations.append(list);
	endInsertRows();
}

void MapLocationModel::clear()
{
	if (!m_mapLocations.size())
		return;
	beginRemoveRows(QModelIndex(), 0, m_mapLocations.size() - 1);
	qDeleteAll(m_mapLocations);
	m_mapLocations.clear();
	endRemoveRows();
}

void MapLocationModel::setSelected(struct dive_site *ds, bool fromClick)
{
	m_selectedDs = ds;
	emit selectedDsChanged();
	if (fromClick)
		emit selectedLocationChanged(getMapLocation(m_selectedDs));
}

QVariant MapLocationModel::selectedDs()
{
	return QVariant::fromValue(m_selectedDs);
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
