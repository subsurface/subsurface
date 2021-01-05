// SPDX-License-Identifier: GPL-2.0
#include "statsgrid.h"
#include "statsaxis.h"
#include "statscolors.h"
#include "zvalues.h"

#include <QChart>
#include <QGraphicsLineItem>

static const double gridWidth = 1.0;
static const Qt::PenStyle gridStyle = Qt::SolidLine;

StatsGrid::StatsGrid(QtCharts::QChart *chart, const StatsAxis &xAxis, const StatsAxis &yAxis)
	: chart(chart), xAxis(xAxis), yAxis(yAxis)
{
}

void StatsGrid::updatePositions()
{
	std::vector<double> xtics = xAxis.ticksPositions();
	std::vector<double> ytics = yAxis.ticksPositions();
	lines.clear();
	if (xtics.empty() || ytics.empty())
		return;

	for (double x: xtics) {
		lines.emplace_back(new QGraphicsLineItem(x, ytics.front(), x, ytics.back(), chart));
		lines.back()->setPen(QPen(gridColor, gridWidth, gridStyle));
		lines.back()->setZValue(ZValues::grid);
	}
	for (double y: ytics) {
		lines.emplace_back(new QGraphicsLineItem(xtics.front(), y, xtics.back(), y, chart));
		lines.back()->setPen(QPen(gridColor, gridWidth, gridStyle));
		lines.back()->setZValue(ZValues::grid);
	}
}
