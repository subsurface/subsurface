// SPDX-License-Identifier: GPL-2.0
#ifndef DIVETOOLTIPITEM_H
#define DIVETOOLTIPITEM_H

#include <QVector>
#include <QPair>
#include <QRectF>
#include <QIcon>
#include <QElapsedTimer>
#include "backend-shared/roundrectitem.h"
#include "core/display.h"

class DiveCartesianAxis;
class QGraphicsLineItem;
class QGraphicsSimpleTextItem;
class QGraphicsPixmapItem;

/* To use a tooltip, simply ->setToolTip on the QGraphicsItem that you want
 * or, if it's a "global" tooltip, set it on the mouseMoveEvent of the ProfileGraphicsView.
 */
class ToolTipItem : public QObject, public RoundRectItem {
	Q_OBJECT
	void updateTitlePosition();
	Q_PROPERTY(QRectF rect READ rect WRITE setRect)

public:
	enum Status {
		COLLAPSED,
		EXPANDED
	};

	explicit ToolTipItem(QGraphicsItem *parent = 0);
	~ToolTipItem();

	void collapse();
	void expand();
	void clear();
	void refresh(const dive *d, const QPointF &pos, bool inPlanner);
	bool isExpanded() const;
	void persistPos();
	void readPos();
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	void setTimeAxis(DiveCartesianAxis *axis);
	void setPlotInfo(const plot_info &plot);
	void clearPlotInfo();
public
slots:
	void setRect(const QRectF &rect);

private:
	typedef QPair<QGraphicsPixmapItem *, QGraphicsSimpleTextItem *> ToolTip;
	QVector<ToolTip> toolTips;
	ToolTip entryToolTip;
	QGraphicsSimpleTextItem *title;
	Status status;
	QRectF rectangle;
	QRectF nextRectangle;
	DiveCartesianAxis *timeAxis;
	plot_info pInfo;
	int lastTime;
	QElapsedTimer refreshTime;
	QList<QGraphicsItem*> oldSelection;

	void addToolTip(const QString &toolTip, const QPixmap &pixmap);
};

#endif // DIVETOOLTIPITEM_H
