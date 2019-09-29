// SPDX-License-Identifier: GPL-2.0
#ifndef GPSLISTMODEL_H
#define GPSLISTMODEL_H

#include "core/gpslocation.h"
#include <QObject>
#include <QAbstractListModel>

class GpsListModel : public QAbstractListModel
{
	Q_OBJECT
public:
	enum GpsListRoles {
		GpsDateRole = Qt::UserRole + 1,
		GpsNameRole,
		GpsLatitudeRole,
		GpsLongitudeRole,
		GpsWhenRole
	};

	static GpsListModel *instance();
	void clear();
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QHash<int, QByteArray> roleNames() const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	void update();
private:
	GpsListModel();
	QVector<gpsTracker> m_gpsFixes;
};

#endif // GPSLISTMODEL_H
