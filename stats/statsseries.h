// SPDX-License-Identifier: GPL-2.0
// Base class for all series. Defines a small virtual interface
// concerning highlighting of series items.
#ifndef STATS_SERIES_H
#define STATS_SERIES_H

#include <QScatterSeries>

namespace QtCharts {
	class QChart;
}
class StatsAxis;

// We derive from a proper scatter series to get access to the map-to
// and map-from coordinates calls. But we don't use any of its functionality.
// This should be removed in due course.

class StatsSeries : public QtCharts::QScatterSeries {
public:
	StatsSeries(QtCharts::QChart *chart, StatsAxis *xAxis, StatsAxis *yAxis);
	virtual ~StatsSeries();
	virtual void updatePositions() = 0;	// Called if chart geometry changes.
	virtual bool hover(QPointF pos) = 0;	// Called on mouse movement. Return true if an item of this series is highlighted.
	virtual void unhighlight() = 0;		// Unhighlight any highlighted item.
};

#endif
