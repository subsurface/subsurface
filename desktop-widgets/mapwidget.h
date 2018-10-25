// SPDX-License-Identifier: GPL-2.0
#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include "core/units.h"
#include <QQuickWidget>
#include <QList>

#undef IGNORE

class QResizeEvent;
class QQuickItem;
class MapWidgetHelper;
struct dive_site;

class MapWidget : public QQuickWidget {

	Q_OBJECT

public:
	MapWidget(QWidget *parent = NULL);
	~MapWidget();

	static MapWidget *instance();
	void reload();

signals:
	void coordinatesChanged(const location_t &);

public slots:
	void centerOnSelectedDiveSite();
	void centerOnDiveSite(struct dive_site *);
	void centerOnIndex(const QModelIndex& idx);
	void endGetDiveCoordinates();
	void repopulateLabels();
	void prepareForGetDiveCoordinates(struct dive_site *ds);
	void selectedDivesChanged(QList<int>);
	void coordinatesChangedLocal(const location_t &);
	void doneLoading(QQuickWidget::Status status);
	void updateDiveSiteCoordinates(struct dive_site *ds, const location_t &);

private:
	static MapWidget *m_instance;
	QQuickItem *m_rootItem;
	MapWidgetHelper *m_mapHelper;
};

#endif // MAPWIDGET_H
