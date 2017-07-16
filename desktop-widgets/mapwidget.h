// SPDX-License-Identifier: GPL-2.0
#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QQuickWidget>

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
	void coordinatesChanged();

public slots:
	void centerOnDiveSite(struct dive_site *);
	void centerOnIndex(const QModelIndex& idx);
	void endGetDiveCoordinates();
	void repopulateLabels();
	void prepareForGetDiveCoordinates();

private:
	static MapWidget *m_instance;
	QQuickItem *m_rootItem;
	MapWidgetHelper *m_mapHelper;

};

#endif // MAPWIDGET_H
