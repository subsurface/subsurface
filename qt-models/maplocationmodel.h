// SPDX-License-Identifier: GPL-2.0
#ifndef MAPLOCATIONMODEL_H
#define MAPLOCATIONMODEL_H

#include <QObject>
#include <QList>
#include <QHash>
#include <QByteArray>
#include <QAbstractListModel>

class MapLocation : public QObject
{
	Q_OBJECT
	Q_PROPERTY(qreal latitude MEMBER m_latitude)
	Q_PROPERTY(qreal longitude MEMBER m_longitude)

public:
	explicit MapLocation();
	explicit MapLocation(qreal lat, qreal lng);

	QVariant getRole(int role) const;

	enum Roles {
		RoleLatitude = Qt::UserRole + 1,
		RoleLongitude
	};

private:
	qreal m_latitude;
	qreal m_longitude;
};

class MapLocationModel : public QAbstractListModel
{
	Q_OBJECT
	Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
	MapLocationModel(QObject *parent = NULL);
	~MapLocationModel();

	Q_INVOKABLE MapLocation *get(int row);

	QVariant data(const QModelIndex &index, int role) const override;
	int rowCount(const QModelIndex &parent) const override;
	int count();
	void add(MapLocation *);
	void clear();

protected:
	QHash<int, QByteArray> roleNames() const;

private:
	QList<MapLocation *> m_mapLocations;
	QHash<int, QByteArray> m_roles;

signals:
	void countChanged(int c);

};

#endif
