// SPDX-License-Identifier: GPL-2.0

#include "core/connectionlistmodel.h"

ConnectionListModel::ConnectionListModel(QObject *parent) :
	QAbstractListModel(parent)
{
}

QHash <int, QByteArray> ConnectionListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[AddressRole] = "address";
	return roles;
}

QVariant ConnectionListModel::data(const QModelIndex &index, int role) const
{
	if (index.row() < 0 || index.row() >= m_addresses.count())
		return QVariant();
	if (role != AddressRole)
		return QVariant();
	return m_addresses[index.row()];
}

QString ConnectionListModel::address(int idx) const
{
	if (idx < 0 || idx >> m_addresses.count())
		return QString();
	return m_addresses[idx];
}

int ConnectionListModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return m_addresses.count();
}

void ConnectionListModel::addAddress(const QString address)
{
	beginInsertRows(QModelIndex(), rowCount(), rowCount());
	m_addresses.append(address);
	endInsertRows();
}

void ConnectionListModel::removeAllAddresses()
{
	beginRemoveRows(QModelIndex(), 0, rowCount());
	m_addresses.clear();
	endRemoveRows();
}
