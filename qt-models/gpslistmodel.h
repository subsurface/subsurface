// SPDX-License-Identifier: GPL-2.0
#ifndef GPSLISTMODEL_H
#define GPSLISTMODEL_H

#include "core/gpslocation.h"
#include "core/singleton.h"
#include <QAbstractListModel>

class GpsListModel : public QAbstractListModel, public SillySingleton<GpsListModel>
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

	GpsListModel(QObject *parent = 0);
	void clear();
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QHash<int, QByteArray> roleNames() const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	void update();
private:
	QVector<gpsTracker> m_gpsFixes;
};

#endif // GPSLISTMODEL_H
