// SPDX-License-Identifier: GPL-2.0
#include "maplocationmodel.h"

MapLocation::MapLocation()
{
}

MapLocation::MapLocation(QGeoCoordinate coord) :
    m_coordinate(coord)
{
}

QVariant MapLocation::getRole(int role) const
{
	switch (role) {
	case Roles::RoleCoordinate:
		return QVariant::fromValue(m_coordinate);
	default:
		return QVariant();
	}
}

MapLocationModel::MapLocationModel(QObject *parent) : QAbstractListModel(parent)
{
	m_roles[MapLocation::Roles::RoleCoordinate] = "coordinate";
}

MapLocationModel::~MapLocationModel()
{
	clear();
}

QVariant MapLocationModel::data( const QModelIndex & index, int role ) const
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

void MapLocationModel::addList(QList<MapLocation *> list)
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
