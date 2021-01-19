// SPDX-License-Identifier: GPL-2.0
#include "quartilemarker.h"
#include "statscolors.h"
#include "statsaxis.h"
#include "zvalues.h"

static const double quartileMarkerSize = 15.0;

QuartileMarker::QuartileMarker(StatsView &view, double pos, double value, StatsAxis *xAxis, StatsAxis *yAxis) :
	ChartLineItem(view, ChartZValue::ChartFeatures, quartileMarkerColor, 2.0),
	xAxis(xAxis), yAxis(yAxis),
	pos(pos),
	value(value)
{
	updatePosition();
}

QuartileMarker::~QuartileMarker()
{
}

void QuartileMarker::updatePosition()
{
	if (!xAxis || !yAxis)
		return;
	double x = xAxis->toScreen(pos);
	double y = yAxis->toScreen(value);
	setLine(QPointF(x - quartileMarkerSize / 2.0, y),
		QPointF(x + quartileMarkerSize / 2.0, y));
}
