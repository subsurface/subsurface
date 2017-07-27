// SPDX-License-Identifier: GPL-2.0
#include "maplocationmodel.h"

const char *MapLocation::PROPERTY_NAME_COORDINATE = "coordinate";
const char *MapLocation::PROPERTY_NAME_UUID       = "uuid";
const char *MapLocation::PROPERTY_NAME_NAME       = "name";

MapLocation::MapLocation()
{
}

MapLocation::MapLocation(quint32 uuid, QGeoCoordinate coord, QString name) :
    m_uuid(uuid), m_coordinate(coord), m_name(name)
{
}

QVariant MapLocation::getRole(int role) const
{
	switch (role) {
	case Roles::RoleUuid:
		return QVariant::fromValue(m_uuid);
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

quint32 MapLocation::uuid()
{
	return m_uuid;
}

MapLocationModel::MapLocationModel(QObject *parent) : QAbstractListModel(parent)
{
	m_roles[MapLocation::Roles::RoleUuid] = MapLocation::PROPERTY_NAME_UUID;
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

int MapLocationModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
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

void MapLocationModel::setSelectedUuid(QVariant uuid, QVariant fromClick)
{
	m_selectedUuid = qvariant_cast<quint32>(uuid);
	const bool fromClickBool = qvariant_cast<bool>(fromClick);
	emit selectedUuidChanged();
	if (fromClickBool)
		emit selectedLocationChanged(getMapLocationForUuid(m_selectedUuid));
}

quint32 MapLocationModel::selectedUuid()
{
	return m_selectedUuid;
}

MapLocation *MapLocationModel::getMapLocationForUuid(quint32 uuid)
{
	MapLocation *location = NULL;
	foreach(location, m_mapLocations) {
		if (location->getRole(MapLocation::Roles::RoleUuid) == uuid)
			break;
	}
	return location;
}
