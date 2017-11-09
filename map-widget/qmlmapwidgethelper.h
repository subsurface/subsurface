// SPDX-License-Identifier: GPL-2.0
#ifndef QMLMAPWIDGETHELPER_H
#define QMLMAPWIDGETHELPER_H

#include <QObject>

class QGeoCoordinate;
class MapLocationModel;
class MapLocation;
struct dive_site;

class MapWidgetHelper : public QObject {

	Q_OBJECT
	Q_PROPERTY(QObject *map MEMBER m_map)
	Q_PROPERTY(MapLocationModel *model MEMBER m_mapLocationModel NOTIFY modelChanged)
	Q_PROPERTY(bool editMode READ editMode WRITE setEditMode NOTIFY editModeChanged)
	Q_PROPERTY(QString pluginObject READ pluginObject NOTIFY pluginObjectChanged)

public:
	explicit MapWidgetHelper(QObject *parent = NULL);

	void centerOnDiveSite(struct dive_site *);
	void reloadMapLocations();
	Q_INVOKABLE void copyToClipboardCoordinates(QGeoCoordinate coord, bool formatTraditional);
	Q_INVOKABLE void calculateSmallCircleRadius(QGeoCoordinate coord);
	Q_INVOKABLE void updateCurrentDiveSiteCoordinatesFromMap(quint32 uuid, QGeoCoordinate coord);
	Q_INVOKABLE void selectVisibleLocations();
	void updateCurrentDiveSiteCoordinatesToMap();
	bool editMode();
	void setEditMode(bool editMode);
	QString pluginObject();

private:
	QObject *m_map;
	MapLocationModel *m_mapLocationModel;
	qreal m_smallCircleRadius;
	QList<int> m_selectedDiveIds;
	bool m_editMode;

private slots:
	void selectedLocationChanged(MapLocation *);

signals:
	void modelChanged();
	void editModeChanged();
	void selectedDivesChanged(QList<int> list);
	void coordinatesChanged();
	void pluginObjectChanged();
};

extern "C" const char *printGPSCoords(int lat, int lon);

#endif
