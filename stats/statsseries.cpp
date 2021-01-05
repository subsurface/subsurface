// SPDX-License-Identifier: GPL-2.0
#include "statsseries.h"
#include "statsaxis.h"

StatsSeries::StatsSeries(QtCharts::QChart *chart, StatsAxis *xAxis, StatsAxis *yAxis) :
	chart(chart), xAxis(xAxis), yAxis(yAxis)
{
}

StatsSeries::~StatsSeries()
{
}

QPointF StatsSeries::toScreen(QPointF p)
{
	return xAxis && yAxis ? QPointF(xAxis->toScreen(p.x()), yAxis->toScreen(p.y()))
			      : QPointF(0.0, 0.0);
}
