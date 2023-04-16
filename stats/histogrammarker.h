// A line to show median an mean in histograms
#ifndef HISTOGRAM_MARKER_H
#define HISTOGRAM_MARKER_H

#include "chartitem.h"

class StatsAxis;

// A line marking median or mean in histograms
class HistogramMarker : public ChartLineItem {
public:
	HistogramMarker(ChartView &view, double val, bool horizontal, QColor color, StatsAxis *xAxis, StatsAxis *yAxis);
	void updatePosition();
private:
	StatsAxis *xAxis, *yAxis;
	double val;
	bool horizontal;
};

#endif
