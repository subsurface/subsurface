// A short line used to mark quartiles
#ifndef QUARTILE_MARKER_H
#define QUARTILE_MARKER_H

#include "chartitem.h"

class StatsAxis;
class StatsView;

class QuartileMarker : public ChartLineItem {
public:
	QuartileMarker(StatsView &view, double pos, double value, StatsAxis *xAxis, StatsAxis *yAxis);
	~QuartileMarker();
	void updatePosition();
private:
	StatsAxis *xAxis, *yAxis;
	double pos, value;
};

#endif
