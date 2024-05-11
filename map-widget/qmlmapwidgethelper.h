// SPDX-License-Identifier: GPL-2.0
#ifndef QMLMAPWIDGETHELPER_H
#define QMLMAPWIDGETHELPER_H

#include "core/units.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include <QObject>
#include <QGeoCoordinate>

#if defined(Q_OS_IOS)
#include <QtPlugin>
Q_IMPORT_PLUGIN(QGeoServiceProviderFactoryGooglemaps)
#endif

#include "qt-models/maplocationmodel.h"
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

	void centerOnSelectedDiveSite();
	Q_INVOKABLE QGeoCoordinate getCoordinates(struct dive_site *ds);
	Q_INVOKABLE void centerOnDiveSite(struct dive_site *ds);
	Q_INVOKABLE void reloadMapLocations();
	Q_INVOKABLE void copyToClipboardCoordinates(QGeoCoordinate coord, bool formatTraditional);
	Q_INVOKABLE void calculateSmallCircleRadius(QGeoCoordinate coord);
	Q_INVOKABLE void updateCurrentDiveSiteCoordinatesFromMap(struct dive_site *ds, QGeoCoordinate coord);
	Q_INVOKABLE void selectVisibleLocations();
	Q_INVOKABLE void selectedLocationChanged(struct dive_site *ds);
	void setSelected(const std::vector<dive_site *> divesites);
	QString pluginObject();
	bool editMode() const;

private:
	void updateEditMode();
	QObject *m_map;
	MapLocationModel *m_mapLocationModel;
	qreal m_smallCircleRadius;
	bool m_editMode;

private slots:
	void diveSiteChanged(struct dive_site *ds, int field);

signals:
	void modelChanged();
	void editModeChanged();
	void selectedDivesChanged(const QList<int> &list);
	void coordinatesChanged(struct dive_site *ds, const location_t &);
	void pluginObjectChanged();
};

#endif
