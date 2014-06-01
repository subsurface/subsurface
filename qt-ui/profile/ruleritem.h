#ifndef RULERITEM_H
#define RULERITEM_H

#include <QObject>
#include <QGraphicsEllipseItem>
#include <QGraphicsObject>
#include "divecartesianaxis.h"
#include "display.h"

struct plot_data;
class RulerItem2;

class RulerNodeItem2 : public QObject, public QGraphicsEllipseItem {
	Q_OBJECT
	friend class RulerItem2;

public:
	explicit RulerNodeItem2();
	void setRuler(RulerItem2 *r);
	void setPlotInfo(struct plot_info &info);
	void recalculate();

protected:
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
private:
	struct plot_info pInfo;
	struct plot_data *entry;
	RulerItem2 *ruler;
	DiveCartesianAxis *timeAxis;
	DiveCartesianAxis *depthAxis;
};

class RulerItem2 : public QObject, public QGraphicsLineItem {
	Q_OBJECT
public:
	explicit RulerItem2();
	void recalculate();

	void setPlotInfo(struct plot_info pInfo);
	RulerNodeItem2 *sourceNode() const;
	RulerNodeItem2 *destNode() const;
	void setAxis(DiveCartesianAxis *time, DiveCartesianAxis *depth);
	void setVisible(bool visible);

public
slots:
	void settingsChanged();

private:
	struct plot_info pInfo;
	QPointF startPoint, endPoint;
	RulerNodeItem2 *source, *dest;
	QString text;
	int height;
	int paint_direction;
	DiveCartesianAxis *timeAxis;
	DiveCartesianAxis *depthAxis;
	QGraphicsRectItem *textItemBack;
	QGraphicsSimpleTextItem *textItem;
};
#endif
