// SPDX-License-Identifier: GPL-2.0
#ifndef MAPLOCATIONMODEL_H
#define MAPLOCATIONMODEL_H

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

	explicit MapLocation();
	explicit MapLocation(struct dive_site *ds, QGeoCoordinate coord, QString name);

	QVariant getRole(int role) const;
	QGeoCoordinate coordinate();
	void setCoordinate(QGeoCoordinate coord);
	void setCoordinateNoEmit(QGeoCoordinate coord);
	QVariant divesiteVariant();
	struct dive_site *divesite();

	enum Roles {
		RoleDivesite = Qt::UserRole + 1,
		RoleCoordinate,
		RoleName
	};

private:
	struct dive_site *m_ds;
	QGeoCoordinate m_coordinate;
	QString m_name;

signals:
	void coordinateChanged();
};

class MapLocationModel : public QAbstractListModel
{
	Q_OBJECT
	Q_PROPERTY(int count READ count NOTIFY countChanged)
	Q_PROPERTY(QVariant selectedDs READ selectedDs NOTIFY selectedDsChanged)

public:
	MapLocationModel(QObject *parent = NULL);
	~MapLocationModel();

	Q_INVOKABLE MapLocation *get(int row);
	QVariant data(const QModelIndex &index, int role) const override;
	int rowCount(const QModelIndex &parent) const override;
	int count();
	void add(MapLocation *);
	void addList(QVector<MapLocation *>);
	void clear();
	MapLocation *getMapLocation(const struct dive_site *ds);
	void updateMapLocationCoordinates(const struct dive_site *ds, QGeoCoordinate coord);
	void setSelected(struct dive_site *ds, bool fromClick = true);
	Q_INVOKABLE void setSelected(QVariant divesite, QVariant fromClick = true);
	QVariant selectedDs();

protected:
	QHash<int, QByteArray> roleNames() const override;

private:
	QVector<MapLocation *> m_mapLocations;
	QHash<int, QByteArray> m_roles;
	struct dive_site *m_selectedDs;

signals:
	void countChanged(int c);
	void selectedDsChanged();
	void selectedLocationChanged(MapLocation *);
};

#endif
