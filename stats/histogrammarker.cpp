// SPDX-License-Identifier: GPL-2.0
#include "histogrammarker.h"
#include "statsaxis.h"
#include "zvalues.h"

static const double histogramMarkerWidth = 2.0;

HistogramMarker::HistogramMarker(ChartView &view, double val, bool horizontal,
				 QColor color, StatsAxis *xAxis, StatsAxis *yAxis) :
	ChartLineItem(view, ChartZValue::ChartFeatures, color, histogramMarkerWidth),
	xAxis(xAxis), yAxis(yAxis),
	val(val), horizontal(horizontal)
{
}

void HistogramMarker::updatePosition()
{
	if (!xAxis || !yAxis)
		return;
	if (horizontal) {
		double y = yAxis->toScreen(val);
		auto [x1, x2] = xAxis->minMaxScreen();
		setLine(QPointF(x1, y), QPointF(x2, y));
	} else {
		double x = xAxis->toScreen(val);
		auto [y1, y2] = yAxis->minMaxScreen();
		setLine(QPointF(x, y1), QPointF(x, y2));
	}
}
