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
	Q_PROPERTY(quint32 uuid READ uuid)
	Q_PROPERTY(QGeoCoordinate coordinate READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)
	Q_PROPERTY(QString name MEMBER m_name)

public:
	static const char *PROPERTY_NAME_COORDINATE;
	static const char *PROPERTY_NAME_UUID;
	static const char *PROPERTY_NAME_NAME;

	explicit MapLocation();
	explicit MapLocation(quint32 uuid, QGeoCoordinate coord, QString name);

	QVariant getRole(int role) const;
	QGeoCoordinate coordinate();
	void setCoordinate(QGeoCoordinate coord);
	quint32 uuid();

	enum Roles {
		RoleUuid = Qt::UserRole + 1,
		RoleCoordinate,
		RoleName
	};

private:
	quint32 m_uuid;
	QGeoCoordinate m_coordinate;
	QString m_name;

signals:
	void coordinateChanged();
};

class MapLocationModel : public QAbstractListModel
{
	Q_OBJECT
	Q_PROPERTY(int count READ count NOTIFY countChanged)
	Q_PROPERTY(quint32 selectedUuid READ selectedUuid NOTIFY selectedUuidChanged)

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
	MapLocation *getMapLocationForUuid(quint32 uuid);
	Q_INVOKABLE void setSelectedUuid(QVariant uuid, QVariant fromClick = true);
	quint32 selectedUuid();

protected:
	QHash<int, QByteArray> roleNames() const;

private:
	QVector<MapLocation *> m_mapLocations;
	QHash<int, QByteArray> m_roles;
	quint32 m_selectedUuid;

signals:
	void countChanged(int c);
	void selectedUuidChanged();
	void selectedLocationChanged(MapLocation *);

};

#endif
