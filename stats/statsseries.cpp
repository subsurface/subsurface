// SPDX-License-Identifier: GPL-2.0
#include "statsseries.h"
#include "statsaxis.h"

StatsSeries::StatsSeries(StatsView &view, StatsAxis *xAxis, StatsAxis *yAxis) :
	view(view), xAxis(xAxis), yAxis(yAxis)
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

bool StatsSeries::supportsLassoSelection() const
{
	return false;
}

void StatsSeries::selectItemsInRect(const QRectF &, SelectionModifier, const std::vector<dive *> &)
{
}
