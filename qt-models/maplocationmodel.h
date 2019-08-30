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
	Q_PROPERTY(QVariant divesite READ divesiteVariant)
	Q_PROPERTY(QGeoCoordinate coordinate READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)
	Q_PROPERTY(QString name MEMBER m_name)

public:
	static const char *PROPERTY_NAME_COORDINATE;
	static const char *PROPERTY_NAME_DIVESITE;
	static const char *PROPERTY_NAME_NAME;
	static const char *PROPERTY_NAME_PIXMAP;

	explicit MapLocation();
	explicit MapLocation(struct dive_site *ds, QGeoCoordinate coord, QString name, bool selected);

	QVariant getRole(int role) const;
	QGeoCoordinate coordinate();
	void setCoordinate(QGeoCoordinate coord);
	void setCoordinateNoEmit(QGeoCoordinate coord);
	QVariant divesiteVariant();
	struct dive_site *divesite();

	enum Roles {
		RoleDivesite = Qt::UserRole + 1,
		RoleCoordinate,
		RoleName,
		RolePixmap
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
	Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
	MapLocationModel(QObject *parent = NULL);
	~MapLocationModel();

	Q_INVOKABLE MapLocation *get(int row);
	QVariant data(const QModelIndex &index, int role) const override;
	int rowCount(const QModelIndex &parent) const override;
	int count();
	void add(MapLocation *);
	// If map is not null, it will be used to place new dive sites without GPS location at the center of the map
	void reload(QObject *map);
	void selectionChanged();
	MapLocation *getMapLocation(const struct dive_site *ds);
	const QVector<dive_site *> &selectedDs() const;
	Q_INVOKABLE void setSelected(struct dive_site *ds);
	// The dive site is passed as a QVariant, because a null-QVariant is not automatically
	// transformed into a null pointer and warning messages are spewed onto the console.
	Q_INVOKABLE bool isSelected(const QVariant &ds) const;

protected:
	QHash<int, QByteArray> roleNames() const override;

private slots:
	void diveSiteChanged(struct dive_site *ds, int field);

private:
	QVector<MapLocation *> m_mapLocations;
	QHash<int, QByteArray> m_roles;
	QVector<dive_site *> m_selectedDs;

signals:
	void countChanged(int c);
};

#endif
