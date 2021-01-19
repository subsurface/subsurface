// A regression line and confidence area
#ifndef REGRESSION_H
#define REGRESSION_H

#include "chartitem.h"

class StatsAxis;
class StatsView;

struct regression_data {
	double a,b;
	double res2, r2, sx2, xavg;
	int n;
};

class RegressionItem : public ChartPixmapItem {
public:
	RegressionItem(StatsView &view, regression_data data, StatsAxis *xAxis, StatsAxis *yAxis);
	~RegressionItem();
	void updatePosition();
	void setFeatures(bool regression, bool confidence);
private:
	StatsAxis *xAxis, *yAxis;
	regression_data reg;
	bool regression, confidence;
};

#endif
