// SPDX-License-Identifier: GPL-2.0
#ifndef DIVETOOLTIPITEM_H
#define DIVETOOLTIPITEM_H

#include <QVector>
#include <QPair>
#include <QRectF>
#include <QIcon>
#include <QElapsedTimer>
#include <QPainter>
#include "backend-shared/roundrectitem.h"
#include "core/profile.h"

struct dive;
class DiveCartesianAxis;
class QGraphicsLineItem;
class QGraphicsSimpleTextItem;
class QGraphicsPixmapItem;

/* To use a tooltip, simply ->setToolTip on the QGraphicsItem that you want
 * or, if it's a "global" tooltip, set it on the mouseMoveEvent of the ProfileGraphicsView.
 */
class ToolTipItem : public QObject, public RoundRectItem {
	Q_OBJECT
	Q_PROPERTY(QRectF rect READ rect WRITE setRect)

public:
	enum Status {
		COLLAPSED,
		EXPANDED
	};

	explicit ToolTipItem(QGraphicsItem *parent = 0);
	~ToolTipItem();

	void refresh(const dive *d, const QPointF &pos, bool inPlanner);
	void readPos();
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	void setTimeAxis(DiveCartesianAxis *axis);
	void setPlotInfo(const plot_info &plot);
	void clearPlotInfo();
	void settingsChanged(bool value);
public
slots:
	void setRect(const QRectF &rect);

private:
	typedef QPair<QGraphicsPixmapItem *, QGraphicsTextItem *> ToolTip;
	QVector<ToolTip> toolTips;
	ToolTip entryToolTip;
	QGraphicsSimpleTextItem *title;
	Status status;
	QPixmap tissues;
	QPainter painter;
	QRectF rectangle;
	QRectF nextRectangle;
	DiveCartesianAxis *timeAxis;
	plot_info pInfo;
	int lastTime;
	QElapsedTimer refreshTime;
	QList<QGraphicsItem*> oldSelection;

	void addToolTip(const QString &toolTip, const QPixmap &pixmap);
	void collapse();
	void expand();
	void clear();
	bool isExpanded() const;
	void persistPos() const;
	void updateTitlePosition();
};

#endif // DIVETOOLTIPITEM_H
