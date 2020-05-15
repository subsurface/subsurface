// SPDX-License-Identifier: GPL-2.0

#include "core/connectionlistmodel.h"
#if defined(BT_SUPPORT)
#include "core/btdiscovery.h"
#endif

ConnectionListModel::ConnectionListModel(QObject *parent) :
	QAbstractListModel(parent)
{
}

QHash <int, QByteArray> ConnectionListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[Qt::DisplayRole] = "display";
	return roles;
}

QVariant ConnectionListModel::data(const QModelIndex &index, int role) const
{
	if (index.row() < 0 || index.row() >= m_addresses.count())
		return QVariant();
	if (role != Qt::DisplayRole)
		return QVariant();
	return m_addresses[index.row()];
}

int ConnectionListModel::rowCount(const QModelIndex&) const
{
	return m_addresses.count();
}

void ConnectionListModel::addAddress(const QString &address)
{
	if (!m_addresses.contains(address)) {
		int idx = rowCount();
#if defined(BT_SUPPORT)
		// make sure that addresses that are just a BT/BLE address without name stay at the end of the list
		if (address != extractBluetoothAddress(address)) {
			for (idx = 0; idx < rowCount(); idx++)
				if (m_addresses[idx] == extractBluetoothAddress(m_addresses[idx]))
					// found the first name-less BT/BLE address, insert before that
					break;
		}
#endif
		beginInsertRows(QModelIndex(), idx, idx);
		m_addresses.insert(idx, address);
		endInsertRows();
	}
}

void ConnectionListModel::removeAllAddresses()
{
	if (rowCount() == 0)
		return;

	beginResetModel();
	m_addresses.clear();
	endResetModel();
}

int ConnectionListModel::indexOf(const QString &address) const
{
	for (int i = 0; i < m_addresses.count(); i++)
		if (m_addresses.at(i).contains(address, Qt::CaseInsensitive))
			return i;
	return -1;
}
