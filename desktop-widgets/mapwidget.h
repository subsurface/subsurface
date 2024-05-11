// SPDX-License-Identifier: GPL-2.0
#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include "core/units.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include <vector>
#include <QQuickWidget>
#include <QList>

#undef IGNORE

class QResizeEvent;
class QQuickItem;
class MapWidgetHelper;

class MapWidget : public QQuickWidget {

	Q_OBJECT

public:
	MapWidget(QWidget *parent = NULL);
	~MapWidget();

	static MapWidget *instance();
	void reload();
	void setSelected(std::vector<dive_site *> divesites);
	bool editMode() const;

public slots:
	void centerOnDiveSite(struct dive_site *);
	void centerOnIndex(const QModelIndex& idx);
	void selectedDivesChanged(const QList<int> &);
	void coordinatesChanged(struct dive_site *ds, const location_t &);
	void doneLoading(QQuickWidget::Status status);
	void divesChanged(const QVector<dive *> &, DiveField field);

private:
	static MapWidget *m_instance;
	QQuickItem *m_rootItem;
	MapWidgetHelper *m_mapHelper;
};

#endif // MAPWIDGET_H
