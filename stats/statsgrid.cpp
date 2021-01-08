// SPDX-License-Identifier: GPL-2.0
#include "statsgrid.h"
#include "statsaxis.h"
#include "statscolors.h"
#include "statshelper.h"
#include "zvalues.h"

#include <QGraphicsLineItem>

static const double gridWidth = 1.0;
static const Qt::PenStyle gridStyle = Qt::SolidLine;

StatsGrid::StatsGrid(QGraphicsScene *scene, const StatsAxis &xAxis, const StatsAxis &yAxis)
	: scene(scene), xAxis(xAxis), yAxis(yAxis)
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
		lines.emplace_back(createItem<QGraphicsLineItem>(scene, x, ytics.front(), x, ytics.back()));
		lines.back()->setPen(QPen(gridColor, gridWidth, gridStyle));
		lines.back()->setZValue(ZValues::grid);
	}
	for (double y: ytics) {
		lines.emplace_back(createItem<QGraphicsLineItem>(scene, xtics.front(), y, xtics.back(), y));
		lines.back()->setPen(QPen(gridColor, gridWidth, gridStyle));
		lines.back()->setZValue(ZValues::grid);
	}
}
