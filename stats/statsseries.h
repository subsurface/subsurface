// SPDX-License-Identifier: GPL-2.0
// Base class for all series. Defines a small virtual interface
// concerning highlighting of series items.
#ifndef STATS_SERIES_H
#define STATS_SERIES_H

#include "statsselection.h"

#include <vector>
#include <QPointF>

class StatsAxis;
class StatsView;
struct dive;
class QRectF;

class StatsSeries {
public:
	StatsSeries(StatsView &view, StatsAxis *xAxis, StatsAxis *yAxis);
	virtual ~StatsSeries();
	virtual void updatePositions() = 0;	// Called if chart geometry changes.
	virtual bool hover(QPointF pos) = 0;	// Called on mouse movement. Return true if an item of this series is highlighted.
	virtual void unhighlight() = 0;		// Unhighlight any highlighted item.
	// Returns true if an item was under the mouse.
	virtual bool selectItemsUnderMouse(const QPointF &pos, SelectionModifier modifier) = 0;
	virtual bool supportsLassoSelection() const;
	// Needs only be defined if supportsLassoSelection() returns true.
	virtual void selectItemsInRect(const QRectF &rect, SelectionModifier modifier, const std::vector<dive *> &oldSelection);
	virtual void divesSelected(const QVector<dive *> &dives) = 0;

protected:
	StatsView &view;
	StatsAxis *xAxis, *yAxis;		// May be zero for charts without axes (pie charts).
	QPointF toScreen(QPointF p);
};

#endif
