// SPDX-License-Identifier: GPL-2.0
#ifndef MAPLOCATIONMODEL_H
#define MAPLOCATIONMODEL_H

#include "core/subsurface-qt/DiveListNotifier.h"
#include <QObject>
#include <QVector>
#include <QHash>
#include <QByteArray>
#include <QAbstractListModel>
#include <QGeoCoordinate>

class MapLocation : public QObject
{
	Q_OBJECT

public:
	explicit MapLocation();
	explicit MapLocation(struct dive_site *ds, QGeoCoordinate coord, QString name, bool selected);

	QVariant getRole(int role) const;
	QGeoCoordinate coordinate();
	void setCoordinate(QGeoCoordinate coord);
	void setCoordinateNoEmit(QGeoCoordinate coord);
	struct dive_site *divesite();

	enum Roles {
		RoleDivesite = Qt::UserRole + 1,
		RoleCoordinate,
		RoleName,
		RolePixmap,
		RoleZ,
		RoleIsSelected
	};

private:
	struct dive_site *m_ds;
	QGeoCoordinate m_coordinate;
	QString m_name;
public:
	bool m_selected = false;

signals:
	void coordinateChanged();
};

class MapLocationModel : public QAbstractListModel
{
	Q_OBJECT

public:
	MapLocationModel(QObject *parent = NULL);
	~MapLocationModel();

	QVariant data(const QModelIndex &index, int role) const override;
	int rowCount(const QModelIndex &parent) const override;
	void add(MapLocation *);
	// If map is not null, it will be used to place new dive sites without GPS location at the center of the map
	void reload(QObject *map);
	void selectionChanged();
	void setSelected(const QVector<dive_site *> &divesites);
	MapLocation *getMapLocation(const struct dive_site *ds);
	const QVector<dive_site *> &selectedDs() const;
	void setSelected(struct dive_site *ds);

protected:
	QHash<int, QByteArray> roleNames() const override;

private slots:
	void diveSiteChanged(struct dive_site *ds, int field);

private:
	QVector<MapLocation *> m_mapLocations;
	QVector<dive_site *> m_selectedDs;
};

#endif
