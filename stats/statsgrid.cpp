// SPDX-License-Identifier: GPL-2.0
#include "statsgrid.h"
#include "chartitem.h"
#include "statsaxis.h"
#include "statscolors.h"
#include "statsview.h"
#include "zvalues.h"

static const double gridWidth = 1.0;

StatsGrid::StatsGrid(StatsView &view, const StatsAxis &xAxis, const StatsAxis &yAxis)
	: view(view), xAxis(xAxis), yAxis(yAxis)
{
}

void StatsGrid::updatePositions()
{
	std::vector<double> xtics = xAxis.ticksPositions();
	std::vector<double> ytics = yAxis.ticksPositions();

	// We probably should be smarter and reuse existing lines.
	// For now, this does it.
	for (auto &line: lines)
		view.deleteChartItem(line);
	lines.clear();

	if (xtics.empty() || ytics.empty())
		return;

	for (double x: xtics) {
		lines.push_back(view.createChartItem<ChartLineItem>(ChartZValue::Grid, gridColor, gridWidth));
		lines.back()->setLine(QPointF(x, ytics.front()), QPointF(x, ytics.back()));
	}
	for (double y: ytics) {
		lines.push_back(view.createChartItem<ChartLineItem>(ChartZValue::Grid, gridColor, gridWidth));
		lines.back()->setLine(QPointF(xtics.front(), y), QPointF(xtics.back(), y));
	}
}
