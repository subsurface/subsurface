// SPDX-License-Identifier: GPL-2.0
#ifndef MAPLOCATIONMODEL_H
#define MAPLOCATIONMODEL_H

#include "core/subsurface-qt/divelistnotifier.h"
#include <vector>
#include <QObject>
#include <QHash>
#include <QByteArray>
#include <QAbstractListModel>
#include <QGeoCoordinate>

class MapLocation
{
public:
	MapLocation(struct dive_site *ds, QGeoCoordinate coord, QString name, bool selected);

	QVariant getRole(int role) const;

	enum Roles {
		RoleDivesite = Qt::UserRole + 1,
		RoleCoordinate,
		RoleName,
		RolePixmap,
		RoleZ,
		RoleIsSelected
	};

	struct dive_site *divesite;
	QGeoCoordinate coordinate;
	QString name;
	bool selected;
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
	void setSelected(const std::vector<dive_site *> &divesites);
	MapLocation *getMapLocation(const struct dive_site *ds); // Attention: not stable!
	const std::vector<dive_site *> &selectedDs() const;
	void setSelected(struct dive_site *ds);

protected:
	QHash<int, QByteArray> roleNames() const override;

private slots:
	void diveSiteChanged(struct dive_site *ds, int field);

private:
	std::vector<MapLocation> m_mapLocations;
	std::vector<dive_site *> m_selectedDs;
};

#endif
