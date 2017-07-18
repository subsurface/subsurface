// SPDX-License-Identifier: GPL-2.0
#ifndef QMLMAPWIDGETHELPER_H
#define QMLMAPWIDGETHELPER_H

#include <QObject>

class MapLocationModel;
class MapLocation;
struct dive_site;

class MapWidgetHelper : public QObject {

	Q_OBJECT
	Q_PROPERTY(QObject *map MEMBER m_map)
	Q_PROPERTY(MapLocationModel *model MEMBER m_mapLocationModel NOTIFY modelChanged)

public:
	explicit MapWidgetHelper(QObject *parent = NULL);

	void centerOnDiveSite(struct dive_site *);
	void reloadMapLocations();

private:
	QObject *m_map;
	MapLocationModel *m_mapLocationModel;

private slots:
	void selectedLocationChanged(MapLocation *);

signals:
	void modelChanged();
};

#endif
