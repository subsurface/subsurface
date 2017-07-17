// SPDX-License-Identifier: GPL-2.0
#ifndef QMLMAPWIDGETHELPER_H
#define QMLMAPWIDGETHELPER_H

#include <QObject>

class MapLocationModel;
struct dive_site;

class MapWidgetHelper : public QObject {

	Q_OBJECT
	Q_PROPERTY(QObject *map MEMBER m_map)
	Q_PROPERTY(MapLocationModel *model MEMBER m_mapLocationModel NOTIFY modelChanged)

public:
	explicit MapWidgetHelper(QObject *parent = NULL);

	void centerOnDiveSite(struct dive_site *);

private:
	QObject *m_map;
	MapLocationModel *m_mapLocationModel;

signals:
	void modelChanged();
};

#endif
