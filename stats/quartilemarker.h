// A short line used to mark quartiles
#ifndef QUARTILE_MARKER_H
#define QUARTILE_MARKER_H

#include "chartitem.h"

class StatsAxis;

class QuartileMarker : public ChartLineItem {
public:
	QuartileMarker(ChartView &view, const StatsTheme &theme, double pos, double value, StatsAxis *xAxis, StatsAxis *yAxis);
	~QuartileMarker();
	void updatePosition();
private:
	StatsAxis *xAxis, *yAxis;
	double pos, value;
};

#endif
