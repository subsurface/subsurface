// SPDX-License-Identifier: GPL-2.0
#ifndef RULERITEM_H
#define RULERITEM_H

#include <QObject>
#include <QGraphicsEllipseItem>
#include <QGraphicsObject>
#include "profile-widget/divecartesianaxis.h"
#include "core/display.h"

struct plot_data;
class RulerItem2;

class RulerNodeItem2 : public QObject, public QGraphicsEllipseItem {
	Q_OBJECT
	friend class RulerItem2;

public:
	explicit RulerNodeItem2();
	void setRuler(RulerItem2 *r);
	void setPlotInfo(const struct plot_info &info);
	void recalculate();

private:
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
	struct plot_info pInfo;
	int idx;
	RulerItem2 *ruler;
	DiveCartesianAxis *timeAxis;
	DiveCartesianAxis *depthAxis;
};

class RulerItem2 : public QObject, public QGraphicsLineItem {
	Q_OBJECT
public:
	explicit RulerItem2();
	void recalculate();

	void setPlotInfo(const struct dive *d, const struct plot_info &pInfo);
	RulerNodeItem2 *sourceNode() const;
	RulerNodeItem2 *destNode() const;
	void setAxis(DiveCartesianAxis *time, DiveCartesianAxis *depth);
	void setVisible(bool visible);

public
slots:
	void settingsChanged(bool toggled);

private:
	const struct dive *dive;
	struct plot_info pInfo;
	QPointF startPoint, endPoint;
	RulerNodeItem2 *source, *dest;
	QString text;
	DiveCartesianAxis *timeAxis;
	DiveCartesianAxis *depthAxis;
	QGraphicsRectItem *textItemBack;
	QGraphicsSimpleTextItem *textItem;
};
#endif
