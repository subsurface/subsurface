// SPDX-License-Identifier: GPL-2.0
// Base class for all series. Defines a small virtual interface
// concerning highlighting of series items.
#ifndef STATS_SERIES_H
#define STATS_SERIES_H

#include <QPointF>

class QGraphicsScene;
class StatsAxis;

class StatsSeries {
public:
	StatsSeries(QGraphicsScene *scene, StatsAxis *xAxis, StatsAxis *yAxis);
	virtual ~StatsSeries();
	virtual void updatePositions() = 0;	// Called if chart geometry changes.
	virtual bool hover(QPointF pos) = 0;	// Called on mouse movement. Return true if an item of this series is highlighted.
	virtual void unhighlight() = 0;		// Unhighlight any highlighted item.
protected:
	QGraphicsScene *scene;
	StatsAxis *xAxis, *yAxis;		// May be zero for charts without axes (pie charts).
	QPointF toScreen(QPointF p);
};

#endif
