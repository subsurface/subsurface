// SPDX-License-Identifier: GPL-2.0
#ifndef MAPLOCATIONMODEL_H
#define MAPLOCATIONMODEL_H

#include <QObject>
#include <QList>
#include <QHash>
#include <QByteArray>
#include <QAbstractListModel>
#include <QGeoCoordinate>

class MapLocation : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QGeoCoordinate coordinate MEMBER m_coordinate)

public:
	explicit MapLocation();
	explicit MapLocation(QGeoCoordinate);

	QVariant getRole(int role) const;

	enum Roles {
		RoleCoordinate = Qt::UserRole + 1,
	};

private:
	QGeoCoordinate m_coordinate;
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
