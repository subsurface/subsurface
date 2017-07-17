// SPDX-License-Identifier: GPL-2.0
#include "maplocationmodel.h"

MapLocation::MapLocation()
{
}

MapLocation::MapLocation(qreal latitude, qreal longitude) :
    m_latitude(latitude), m_longitude(longitude)
{
}

QVariant MapLocation::getRole(int role) const
{
	switch (role) {
	case Roles::RoleLatitude:
		return m_latitude;
	case Roles::RoleLongitude:
		return m_longitude;
	default:
		return QVariant();
	}
}

MapLocationModel::MapLocationModel(QObject *parent) : QAbstractListModel(parent)
{
	m_roles[MapLocation::Roles::RoleLatitude] = "latitude";
	m_roles[MapLocation::Roles::RoleLongitude] = "longitude";
}

MapLocationModel::~MapLocationModel()
{
	qDeleteAll(m_mapLocations);
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
