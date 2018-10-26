// SPDX-License-Identifier: GPL-2.0
#ifndef QMLMAPWIDGETHELPER_H
#define QMLMAPWIDGETHELPER_H

#include "core/units.h"
#include <QObject>
#include <QGeoCoordinate>
#include <QVariant>

#if defined(Q_OS_IOS)
#include <QtPlugin>
Q_IMPORT_PLUGIN(QGeoServiceProviderFactoryGooglemaps)
#endif

class MapLocationModel;
class MapLocation;
struct dive_site;

class MapWidgetHelper : public QObject {

	Q_OBJECT
	Q_PROPERTY(QObject *map MEMBER m_map)
	Q_PROPERTY(MapLocationModel *model MEMBER m_mapLocationModel NOTIFY modelChanged)
	Q_PROPERTY(bool editMode MEMBER m_editMode NOTIFY editModeChanged)
	Q_PROPERTY(QString pluginObject READ pluginObject NOTIFY pluginObjectChanged)

public:
	explicit MapWidgetHelper(QObject *parent = NULL);

	void centerOnDiveSite(struct dive_site *);
	void centerOnSelectedDiveSite();
	Q_INVOKABLE QGeoCoordinate getCoordinates(QVariant dive_site);
	Q_INVOKABLE void centerOnDiveSite(QVariant dive_site);
	Q_INVOKABLE void reloadMapLocations();
	Q_INVOKABLE void copyToClipboardCoordinates(QGeoCoordinate coord, bool formatTraditional);
	Q_INVOKABLE void calculateSmallCircleRadius(QGeoCoordinate coord);
	Q_INVOKABLE void updateCurrentDiveSiteCoordinatesFromMap(struct dive_site *ds, QGeoCoordinate coord);
	Q_INVOKABLE void selectVisibleLocations();
	void updateDiveSiteCoordinates(struct dive_site *ds, const location_t &);
	void enterEditMode(struct dive_site *ds);
	void exitEditMode();
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
	void coordinatesChanged(const location_t &);
	void pluginObjectChanged();
};

#endif
